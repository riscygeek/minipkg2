#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fmt/core.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "minipkg2.hpp"
#include "package.hpp"
#include "utils.hpp"

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
            pkg.provided_by = {};
            pkg.sources     = read_lines(file);
            pkg.bdepends    = read_lines(file);
            pkg.rdepends    = read_lines(file);
            pkg.provides    = read_lines(file);
            pkg.conflicts   = read_lines(file);
            pkg.features    = read_lines(file);

            ::fclose(file);

            return pkg;
        }
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
            if (stat(path.c_str(), &st) != 0 || (st.st_mode & S_IFMT) != S_IFDIR)
                continue;

            pkgs.push_back(package::parse(fmt::format("{}/{}", path, filename)));
            pkgs.back().from = from;
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
}

std::ostream& operator<<(std::ostream& stream, const minipkg2::package& pkg) {
    const auto print_line = [&](std::string_view name, std::size_t n, const std::string* strs) {
        stream << name << std::string(30 - name.length(), ' ');
        for (std::size_t i = 0; i < n; ++i) {
            stream << strs[i];
        }
        stream << '\n';
    };
    print_line("Name",                  1,                      &pkg.name);
    print_line("Version",               1,                      &pkg.version);
    print_line("URL",                   1,                      &pkg.url);
    print_line("Description",           1,                      &pkg.description);
    print_line("Sources",               pkg.sources.size(),     pkg.sources.data());
    print_line("Build Dependencies",    pkg.bdepends.size(),    pkg.bdepends.data());
    print_line("Runtime Dependencies",  pkg.rdepends.size(),    pkg.rdepends.data());
    print_line("Provides",              pkg.provides.size(),    pkg.provides.data());
    print_line("Conflicts",             pkg.conflicts.size(),   pkg.conflicts.data());
    print_line("Features",              pkg.features.size(),    pkg.features.data());
    return stream;
}
