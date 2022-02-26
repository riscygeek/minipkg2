#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fmt/core.h>
#include <functional>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "minipkg2.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "print.hpp"
#include "git.hpp"

#define SHELL_SCRIPT_HEADER                              \
   "[[ -f " ENV_FILE " ]] && source \"" ENV_FILE "\"\n"  \
   "source \"$pkgfile\"\n"

namespace minipkg2 {
    using namespace std::literals;
    bool package::is_installed() const {
        return false;
    }


    package package::parse(const std::string& filename) {
        if (::access(filename.c_str(), R_OK) != 0)
            throw std::runtime_error(fmt::format("Cannot access '{}'.", filename));

        int pipefd[2];

        if (::pipe(pipefd) != 0) {
            throw std::runtime_error("Failed to create a pipe.");
        }

        const ::pid_t pid = ::vfork();
        if (pid < 0) {
            throw std::runtime_error("Failed to vfork().");
        } if (pid == 0) {
            // Setup the environment.
            ::setenv("ROOT",        rootdir.c_str(),    1);
            ::setenv("ENV_FILE",    env_filename,       1);
            ::setenv("MINIPKG2",    self.c_str(),       1);

            // Close the read-end of the pipe.
            ::close(pipefd[0]);

            // Redirect stdin to /dev/null
            ::close(STDIN_FILENO);
            if (::open("/dev/null", O_RDONLY) != STDIN_FILENO)
                ::_exit(100);

            // Redirect stdout to minipkg2
            ::close(STDOUT_FILENO);
            if (::dup(pipefd[1]) != STDOUT_FILENO)
                ::_exit(100);

            ::execlp(CONFIG_SHELL, CONFIG_SHELL, parse_filename, filename.c_str(), nullptr);
            ::_exit(100);
        } else {
            // Close write-end of the pipe.
            ::close(pipefd[1]);

            // Wait for the program to exit.
            int wstatus;
            if (::waitpid(pid, &wstatus, 0) != pid)
                throw std::runtime_error(fmt::format("Failed to wait for process {}.", pid));

            if (!WIFEXITED(wstatus))
                throw std::runtime_error(fmt::format("Process {} did not exit.", pid));

            if (const int ec = WEXITSTATUS(wstatus); ec != 0) {
                std::string msg;
                if (ec == 100) {
                    msg = fmt::format("Process {} failed to redirect its IO.", pid);
                } else {
                    msg = fmt::format("Process {} terminated with exit code {}.", pid, ec);
                }
                throw std::runtime_error(msg);
            }

            FILE* file = ::fdopen(pipefd[0], "r");
            if (!file)
                throw std::runtime_error("Failed to fdopen() pipe.");

            const auto read_lines = [](FILE* file) {
                std::vector<std::string> lines{};
                std::string line;
                while (!std::feof(file) && (line = freadline(file)) != "--"sv) {
                    lines.push_back(line);
                }
                return lines;
            };

            // Parse the output.
            package pkg;
            pkg.from        = package::source::OTHER;
            pkg.filename    = filename;
            pkg.name        = freadline(file);
            pkg.version     = freadline(file);
            pkg.url         = freadline(file);
            pkg.description = freadline(file);
            pkg.sources     = read_lines(file);
            pkg.bdepends    = read_lines(file);
            pkg.rdepends    = read_lines(file);
            pkg.provides    = read_lines(file);
            pkg.conflicts   = read_lines(file);
            pkg.features    = read_lines(file);

            ::fclose(file);

            struct stat st;
            if (::lstat(filename.c_str(), &st) != 0 && S_ISLNK(st.st_mode)) {
                pkg.provided_by = xreadlink(filename);
            }

            return pkg;
        }
    }
    package package::parse(source from, std::string_view name) {
        std::string path;
        switch (from) {
        case source::LOCAL:
            path = fmt::format("{}/{}/package.info", pkgdir, name);
            break;
        case source::REPO:
            path = fmt::format("{}/{}/package.build", repodir, name);
            break;
        default:
            throw std::runtime_error("Invalid package source");
        }
        return parse(path);
    }
    static std::vector<package> parse_all(const std::string& dirname, std::string_view filename, package::source from) {
        std::vector<package> pkgs{};
        ::DIR* dir = ::opendir(dirname.c_str());
        if (!dir)
            throw std::runtime_error(fmt::format("Failed to open directory '{}'.", dirname));

        struct ::dirent* ent;
        while ((ent = ::readdir(dir)) != nullptr) {
            if (ent->d_name[0] == '.')
                continue;
            const auto path = fmt::format("{}/{}", dirname, ent->d_name);

            struct ::stat st;
            if (::stat(path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
                continue;

            auto pkg = package::parse(fmt::format("{}/{}", path, filename));
            pkg.from = from;
            pkgs.push_back(std::move(pkg));
        }
        ::closedir(dir);
        return pkgs;
    }
    std::vector<package> package::parse_local() {
        return parse_all(pkgdir, "package.info"sv, source::LOCAL);
    }
    std::vector<package> package::parse_repo() {
        return parse_all(repodir, "package.build"sv, source::REPO);
    }

    bool package::download() const {
        bool success = true;
        for (const auto& src : sources) {
            const auto end = src.rfind('/');
            if (end == std::string::npos) {
                printerr(color::ERROR, "{}: Invalid URL '{}'", name, src);
                success = false;
                continue;
            }
            auto dest = fmt::format("{}/{}-{}/src/{}", builddir, name, version, src.substr(end + 1));

            if (starts_with(src, "git://")) {
                // Remove trailing '.git'
                if (ends_with(dest, ".git")) {
                    dest = dest.substr(0, dest.size() - 4);
                }

                success &= git::sync(src, dest);
            } else {
                success &= minipkg2::download(src, dest);
            }
        }
        return success;
    }
    std::vector<package> package::resolve(const std::vector<std::string>& rnames, bool resolve_deps, bool skip_installed) {
        std::vector<package> pkgs{};

        const auto has = [&pkgs, &skip_installed](std::string_view name) {
            if (skip_installed && is_installed(name))
                return true;
            for (const auto& pkg : pkgs) {
                if (name == pkg.name)
                    return true;
            }
            return false;
        };

        const std::function<void(const std::vector<std::string>&)> do_resolve = [&](const std::vector<std::string>& names) -> void {
            for (const auto& name : names) {
                if (has(name))
                    continue;

                auto pkg = parse(source::REPO, name);
                pkg.from = source::REPO;
                if (resolve_deps) {
                    do_resolve(pkg.bdepends);
                    pkgs.push_back(pkg);
                    do_resolve(pkg.rdepends);
                } else {
                    pkgs.push_back(std::move(pkg));
                }
            }
        };

        do_resolve(rnames);

        return pkgs;
    }
    std::string package::make_pkglist(const std::vector<package>& pkgs, bool include_version) {
        std::string str{};
        for (const auto& pkg : pkgs) {
            str += ' ';
            str += fmt::format(include_version ? "{:v}": "{}", pkg);
        }
        return str;
    }
    bool package::is_installed(std::string_view name) {
        const auto path = fmt::format("{}/{}", pkgdir, name);
        struct ::stat st;
        // TODO: Should stat() or lstat() be used here???
        return ::stat(path.c_str(), &st) == 0;
    }
}
