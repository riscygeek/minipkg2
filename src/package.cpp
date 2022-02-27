#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fmt/core.h>
#include <functional>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <spawn.h>
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
        xpipe(pipefd);

        ::posix_spawn_file_actions_t actions;
        ::posix_spawn_file_actions_init(&actions);
        ::posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
        ::posix_spawn_file_actions_addclose(&actions, pipefd[0]);
        ::posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);

        std::vector<char*> args{};
        args.push_back(xstrdup("bash"));
        args.push_back(xstrdup(parse_filename));
        args.push_back(xstrdup(filename));
        args.push_back(nullptr);

        std::vector<char*> env = copy_environ();
        add_environ(env, "ROOT",        rootdir);
        add_environ(env, "ENV_FILE",    env_filename);
        add_environ(env, "MINIPKG2",    self);
        env.push_back(nullptr);

        ::pid_t pid;
        if (::posix_spawnp(&pid, "bash", &actions, nullptr, args.data(), env.data()) != 0)
            raise("Failed to posix_spawn() the shell.");

        if (const int ec = xwait(pid); ec != 0)
            raise("{}: Process {} terminated with exit code {}.", filename, pid, ec);

        // Cleanup.
        xclose(pipefd[1]);
        free_environ(args);
        free_environ(env);
        ::posix_spawn_file_actions_destroy(&actions);

        FILE* file = ::fdopen(pipefd[0], "r");
        if (!file)
            raise("Failed to fdopen() pipe.");

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
    static std::set<package> parse_all(const std::string& dirname, std::string_view filename, package::source from) {
        std::set<package> pkgs{};
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
            pkgs.insert(std::move(pkg));
        }
        ::closedir(dir);
        return pkgs;
    }
    const std::set<package>& package::parse_local() {
        if (local_packages.empty()) {
            local_packages = parse_all(pkgdir, "package.info"sv, source::LOCAL);
        }
        return local_packages;
    }
    const std::set<package>& package::parse_repo() {
        if (repo_packages.empty()) {
            repo_packages = parse_all(repodir, "package.build"sv, source::REPO);
        }
        return repo_packages;
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
    std::vector<package> package::resolve(const std::vector<std::string>& rnames, bool resolve_deps, skip_policy skip) {
        std::vector<package> pkgs{};

        const auto has = [&pkgs](std::string_view name, bool skip_installed) {
            if (skip_installed && is_installed(name))
                return true;
            for (const auto& pkg : pkgs) {
                if (pkg.is_provided(name))
                    return true;
            }
            return false;
        };

        std::function<void(const std::vector<std::string>&)> do_resolve;

        const auto add = [&](std::string_view name) {
            auto pkg = parse(source::REPO, name);
            pkg.from = source::REPO;
            if (resolve_deps) {
                do_resolve(pkg.bdepends);
                pkgs.push_back(pkg);
                do_resolve(pkg.rdepends);
            } else {
                pkgs.push_back(std::move(pkg));
            }
        };

        do_resolve = [&](const std::vector<std::string>& names) -> void {
            for (const auto& name : names) {
                if (has(name, skip == skip_policy::DEEP))
                    continue;
                add(name);
            }
        };

        for (const auto& name : rnames) {
            if (has(name, skip != skip_policy::NEVER))
                continue;
            add(name);
        }

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
    bool package::is_provided(std::string_view n) const {
        return n == name || std::find(provides.begin(), provides.end(), n) != provides.end();
    }
    bool package::conflicts_with(std::string_view n) const {
        return is_provided(n) && std::find(conflicts.begin(), conflicts.end(), n) != conflicts.end();
    }
    bool package::check_conflicts() const {
        bool success = true;
        for (const auto& c : conflicts) {
            if (is_installed(c)) {
                printerr(color::ERROR, "Package '{}' conflicts with '{}'", name, c);
                success = false;
            }
        }
        const auto& locals = parse_local();
        for (const auto& pkg : locals) {
            if (pkg.conflicts_with(name)) {
                printerr(color::ERROR, "Package '{}' conflicts with '{}'", pkg.name, name);
                success = false;
            }
        }
        return success;
    }
    bool package::download(const std::vector<package>& pkgs) {
        bool success = true;
        for (std::size_t i = 0; i < pkgs.size(); ++i) {
            const auto& pkg = pkgs[i];
            printerr(color::LOG, "({}/{}) Downloading {:v}", i+1, pkgs.size(), pkg);

            success &= pkg.download();
        }
        return success;
    }
    std::optional<bin_package> package::build(std::string_view binpkg, const std::string& filesdir) const {
        const auto path_basedir     = fmt::format("{}/{}-{}", builddir, name, version);
        const auto path_srcdir      = path_basedir + "/src";
        const auto path_builddir    = path_basedir + "/build";
        const auto path_pkgdir      = path_basedir + "/pkg";
        const auto path_logfile     = path_basedir + "/log";
        const auto path_metadir     = path_pkgdir  + "/.meta";

        mkdir_p(path_builddir);
        rm_rf(path_pkgdir);
        mkdir_p(path_pkgdir);

        int pipefd[2];
        xpipe(pipefd);

        ::posix_spawn_file_actions_t actions;
        ::posix_spawn_file_actions_init(&actions);
        ::posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
        ::posix_spawn_file_actions_addclose(&actions, pipefd[0]);
        ::posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
        ::posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);

        std::vector<char*> args{};
        args.push_back(xstrdup("bash"));
        args.push_back(xstrdup(build_filename));
        args.push_back(xstrdup(filename));
        args.push_back(nullptr);

        std::vector<char*> env = copy_environ();
        add_environ(env, "ROOT",        rootdir);
        add_environ(env, "ENV_FILE",    env_filename);
        add_environ(env, "MINIPKG2",    self);
        add_environ(env, "HOST",        host);
        add_environ(env, "JOBS",        fmt::format("{}", jobs));
        add_environ(env, "pkgfile",     filename);
        add_environ(env, "srcdir",      path_srcdir);
        add_environ(env, "builddir",    path_builddir);
        add_environ(env, "pkgdir",      path_pkgdir);
        add_environ(env, "filesdir",    filesdir);

        ::pid_t pid;
        if (::posix_spawnp(&pid, "bash", &actions, nullptr, args.data(), env.data()) != 0)
            raise("Failed to posix_spawn() the shell.");

        // Cleanup.
        xclose(pipefd[1]);
        free_environ(args);
        free_environ(env);
        ::posix_spawn_file_actions_destroy(&actions);

        FILE* log = ::fdopen(pipefd[0], "r");
        assert(log != nullptr);

        FILE* logfile = std::fopen(path_logfile.c_str(), "w");
        if (!logfile) {
            printerr(color::WARN, "Failed to open log file ({}) for package {}.", path_logfile, name);
        }

        char buffer[100];
        while (std::fgets(buffer, sizeof(buffer) - 1, log) != nullptr) {
            if (verbosity >= verbosity_level::VERBOSE)
                std::fputs(buffer, stderr);
            std::fputs(buffer, logfile);
        }

        std::fclose(logfile);
        std::fclose(log);

        if (const int ec = xwait(pid); ec != 0) {
            if (verbosity < verbosity_level::VERBOSE)
                cat(stderr, path_logfile);
            printerr(color::ERROR, "Failed to build package '{:v}'. Log file: '{}'.", *this, path_logfile);
            return {};
        }

        auto info = package_info::from(*this);

        mkdir_p(path_metadir);
        info.write_to(path_metadir + "/package.info");


        const auto cmd = fmt::format("cd '{}' && fakeroot tar -caf '{}' .", path_pkgdir, binpkg);
        if (std::system(cmd.c_str()) != 0) {
            printerr(color::ERROR, "Can't create binary package.");
            return {};
        }
        return bin_package{ std::string(binpkg), std::move(info) };
    }
    package_info package_info::from(const package& pkg) {
        return package_info{ pkg.name, pkg.version, pkg.url, pkg.description, pkg.rdepends, pkg.provides, pkg.conflicts };
    }
    bool package_info::write_to(const std::string& filename) const {
        std::FILE* file = std::fopen(filename.c_str(), "w");
        if (!file)
            return false;

        const auto print_vec = [file](std::string_view name, const std::vector<std::string>& vec) {
            if (vec.empty()) {
                fmt::print(file, "{}=()\n", name);
                return;
            }
            fmt::print(file, "{}=('{}'", name, vec.front());
            for (auto it = vec.begin()+1; it != vec.end(); ++it) {
                fmt::print(file, " '{}'", *it);
            }
            fmt::print(file, ")\n");
        };

        fmt::print(file, "# This file is generated by minipkg2.\n");
        fmt::print(file, "name='{}'\n",          name);
        fmt::print(file, "pkgver='{}'\n",        version);
        fmt::print(file, "url='{}'\n",           url);
        fmt::print(file, "description='{}'\n",   description);
        print_vec("rdepends",   rdepends);
        print_vec("provides",   provides);
        print_vec("conflicts",  conflicts);
        std::fclose(file);
        return true;
    }
    package_info package_info::parse_file(const std::string& filename) {
        // TODO: Write a parser.
        return from(package::parse(filename));
    }
    package_info package_info::parse_local(const std::string& name) {
        return from(package::parse(package::source::LOCAL, name));
    }
}
