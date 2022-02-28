#include <sys/types.h>
#include <sys/stat.h>
#include <fmt/core.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <spawn.h>
#include <cassert>
#include <map>
#include "minipkg2.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "print.hpp"
#include "git.hpp"

namespace minipkg2 {
    using namespace std::literals;
    // print()
    static void print_line(std::string_view name, std::size_t n, const std::string* strs) {
        fmt::print("{:30}:", name);
        for (std::size_t i = 0; i < n; ++i)
            fmt::print(" {}", strs[i]);
        fmt::print("\n");
    }
    void package_base::print() const {
        print_line("Name",                  1,                  &name);
        print_line("Version",               1,                  &version);
        print_line("URL",                   1,                  &url);
        print_line("Description",           1,                  &description);
        print_line("Runtime Dependencies",  rdepends.size(),    rdepends.data());
        print_line("Provides",              provides.size(),    provides.data());
        print_line("Conflicts",             conflicts.size(),   conflicts.data());
    }
    void source_package::print() const {
        package_base::print();
        print_line("Sources",               sources.size(),     sources.data());
        print_line("Build Dependencies",    bdepends.size(),    bdepends.data());
        print_line("Features",              features.size(),    features.data());
    }
    void binary_package_info::print() const {
        package_base::print();
        const auto bdate = uts_to_str(build_date);
        print_line("Build Date",            1,                  &bdate);
    }
    void installed_package::print() const {
        binary_package_info::print();
        const auto idate = uts_to_str(install_date);
        print_line("Install Date",          1,                  &idate);
    }

    // to_file()
    static std::string print_vec(std::string_view name, const std::vector<std::string>& vec) {
        if (vec.empty()) {
            return fmt::format("{}=()\n", name);
        }
        std::string str = fmt::format("{}=('{}'", name, vec.front());
        for (auto it = vec.begin() + 1; it != vec.end(); ++it) {
            str += fmt::format(" '{}'", *it);
        }
        return str + ")\n";
    }
    std::string binary_package_info::to_file() const {
        std::string str{};
        str += "# This file was generated by minipkg2.\n";
        str += fmt::format("pkgname='{}'\n",           name);
        str += fmt::format("pkgver='{}'\n",         version);
        str += fmt::format("url='{}'\n",            url);
        str += fmt::format("description='{}'\n",    description);
        str += print_vec("rdepends",                rdepends);
        str += print_vec("provides",                provides);
        str += print_vec("conflicts",               conflicts);
        str += fmt::format("build_date='{}'\n",     build_date);
        return str;
    }
    std::string installed_package::to_file() const {
        return fmt::format("{}install_date='{}'\n", binary_package_info::to_file(), install_date);
    }

    // parse()
    struct generic_package {
        std::string filename;
        std::string name;
        std::string version;
        std::string url;
        std::string description;
        std::vector<std::string> sources;
        std::vector<std::string> bdepends;
        std::vector<std::string> rdepends;
        std::vector<std::string> provides;
        std::vector<std::string> conflicts;
        std::vector<std::string> features;
        std::time_t build_date;
        std::time_t install_date;
        std::string provided_by;
    };
    static std::optional<generic_package> parse_generic(const std::string& filename) {
        printerr(color::DEBUG, "Parsing '{}'...", filename);
        if (::access(filename.c_str(), R_OK) != 0) {
            printerr(color::DEBUG, "Cannot access '{}'.", filename);
            return {};
        }

        int pipefd[2];
        xpipe(pipefd);

        // Construct the file descriptor description.
        ::posix_spawn_file_actions_t actions;
        ::posix_spawn_file_actions_init(&actions);
        ::posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
        ::posix_spawn_file_actions_addclose(&actions, pipefd[0]);
        ::posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
        ::posix_spawn_file_actions_addclose(&actions, pipefd[1]);

        // Construct argv.
        std::vector<char*> args{};
        args.push_back(xstrdup("bash"));
        args.push_back(xstrdup(parse_filename));
        args.push_back(xstrdup(filename));
        args.push_back(nullptr);

        // Construct envp.
        std::vector<char*> env = copy_environ();
        add_environ(env, "ROOT",        rootdir);
        add_environ(env, "ENV_FILE",    env_filename);
        add_environ(env, "MINIPKG2",    self);
        env.push_back(nullptr);

        ::pid_t pid;
        if (::posix_spawnp(&pid, "bash", &actions, nullptr, args.data(), env.data()) != 0)
            raise("{}: parse(): Failed to posix_spawn() the shell.", filename);

        // Cleanup.
        xclose(pipefd[1]);
        free_environ(args);
        free_environ(env);
        ::posix_spawn_file_actions_destroy(&actions);

        // Read the package information.
        std::FILE* file = ::fdopen(pipefd[0], "r");
        if (!file)
            raise("Failed to smoke on pipe.");

        const auto read_lines = [file] {
            std::vector<std::string> lines{};
            std::string line;
            while (!std::feof(file) && (line = freadline(file)) != "--"sv) {
                lines.push_back(std::move(line));
            }
            return lines;
        };

        // Parse the output.
        generic_package pkg;
        pkg.filename        = filename;
        pkg.name            = freadline(file);
        pkg.version         = freadline(file);
        pkg.url             = freadline(file);
        pkg.description     = freadline(file);
        pkg.sources         = read_lines();
        pkg.bdepends        = read_lines();
        pkg.rdepends        = read_lines();
        pkg.provides        = read_lines();
        pkg.conflicts       = read_lines();
        pkg.features        = read_lines();
        pkg.build_date      = str_to_uts(freadline(file));
        pkg.install_date    = str_to_uts(freadline(file));

        std::fclose(file);

        if (const int ec = xwait(pid); ec != 0) {
            printerr(color::ERROR, "{}: Shell terminated with exit code {}.", pkg.name, ec);
            return {};
        }

        struct ::stat st;
        if (::lstat(filename.c_str(), &st) == 0 && S_ISLNK(st.st_mode)) {
            pkg.provided_by = xreadlink(filename);
        }

        return pkg;
    }
    static void generic_to_base(generic_package& generic, package_base& base) {
        base.filename       = std::move(generic.filename);
        base.name           = std::move(generic.name);
        base.version        = std::move(generic.version);
        base.url            = std::move(generic.url);
        base.description    = std::move(generic.description);
        base.rdepends       = std::move(generic.rdepends);
        base.provides       = std::move(generic.provides);
        base.conflicts      = std::move(generic.conflicts);
        base.provided_by    = std::move(generic.provided_by);
    }
    std::optional<source_package> source_package::parse_file(const std::string& filename) {
        auto result = parse_generic(filename);
        if (!result.has_value())
            return {};
        auto& generic = result.value();
        source_package pkg;

        generic_to_base(generic, pkg);
        pkg.sources         = std::move(generic.sources);
        pkg.bdepends        = std::move(generic.bdepends);
        pkg.features        = std::move(generic.features);

        return pkg;
    }
    std::optional<binary_package_info> binary_package_info::parse_file(const std::string& filename) {
        auto result = parse_generic(filename);
        if (!result.has_value())
            return {};

        auto& generic = result.value();
        binary_package_info pkg;

        generic_to_base(generic, pkg);
        pkg.build_date      = generic.build_date;

        return pkg;
    }

    std::optional<installed_package> installed_package::parse_file(const std::string& filename) {
        auto result = parse_generic(filename);
        if (!result.has_value())
            return {};

        auto& generic = result.value();
        installed_package pkg;

        generic_to_base(generic, pkg);
        pkg.build_date      = generic.build_date;
        pkg.install_date    = generic.install_date;

        return pkg;
    }
    std::optional<source_package> source_package::parse_repo(std::string_view name) {
        return parse_file(fmt::format("{}/{}/package.build", repodir, name));
    }
    std::optional<installed_package> installed_package::parse_local(std::string_view name) {
        return parse_file(fmt::format("{}/{}/package.info", pkgdir, name));
    }
    template<class T>
    static std::set<T> do_parse(const std::string& dirname, std::string_view filename) {
        std::set<T> pkgs{};
        ::DIR* dir = ::opendir(dirname.c_str());
        if (!dir)
            raise("Failed to open directory '{}'.", dirname);

        struct ::dirent* ent;
        while ((ent = ::readdir(dir)) != nullptr) {
            if (ent->d_name[0] == '.')
                continue;

            const auto path = fmt::format("{}/{}/{}", dirname, ent->d_name, filename);

            if (::access(path.c_str(), R_OK) != 0)
                continue;

            auto result = T::parse_file(path);
            if (result.has_value()) {
                pkgs.insert(std::move(result.value()));
            } else {
                printerr(color::WARN, "Failed to parse package '{}'.", path);
            }
        }
        return pkgs;
    }
    std::set<source_package> source_package::parse_repo() {
        return do_parse<source_package>(repodir, "package.build");
    }
    std::set<installed_package> installed_package::parse_local() {
        return do_parse<installed_package>(pkgdir, "package.info");
    }
    bool source_package::check_conflicts() const {
        // TODO
        printerr(color::WARN, "Checking for conflicts is not implemented.");
        return true;
    }
    bool source_package::download() const {
        bool success = true;
        for (const auto& src : sources) {
            const auto end = src.rfind('/');
            if (end == std::string::npos) {
                printerr(color::ERROR, "{}: Invalid URL '{}'.", name, src);
                success = false;
                continue;
            }

            auto dest = fmt::format("{}/{}-{}/src/{}", builddir, name, version, src.substr(end + 1));

            if (starts_with(src, "git://")) {
                // Remove trailing '.git'
                if (ends_with(dest, ".git"))
                    dest = dest.substr(0, dest.size() - 4);

                success &= git::sync(src, dest);
            } else {
                success &= minipkg2::download(src, dest);
            }
        }
        return success;
    }


    // Utility functions.
    bool source_package::download(const std::vector<source_package>& pkgs) {
        bool success = true;
        for (std::size_t i = 0; i < pkgs.size(); ++i) {
            const auto& pkg = pkgs[i];
            printerr(color::LOG, "({}/{}) Downloading {:v}...", i+1, pkgs.size(), pkg);

            success &= pkg.download();
        }
        return success;
    }
    std::vector<source_package> source_package::resolve(const std::vector<std::string>& args, bool resolve_deps, resolve_skip_policy policy) {
        std::vector<source_package> pkgs{};

        const auto has = [&pkgs](std::string_view name, bool skip_installed) {
            if (skip_installed && installed_package::is_installed(name))
                return true;
            for (const auto& pkg : pkgs) {
                if (pkg.is_provider_of(name))
                    return true;
            }
            return false;
        };

        std::function<void(const std::vector<std::string>&)> do_resolve;

        const auto add = [&](std::string_view name) {
            auto result = source_package::parse_repo(name);
            if (!result.has_value())
                raise("Invalid package: {}", name);
            auto& pkg = result.value();
            if (resolve_deps) {
                do_resolve(pkg.bdepends);
                pkgs.push_back(pkg);
                do_resolve(pkg.rdepends);
            } else {
                pkgs.push_back(std::move(pkg));
            }
        };

        do_resolve = [&](const std::vector<std::string>& names) {
            for (const auto& name : names) {
                if (has(name, policy != resolve_skip_policy::NEVER))
                    continue;
                add(name);
            }
        };

        for (const auto& name : args) {
            if (has(name, policy == resolve_skip_policy::ALWAYS))
                continue;
            add(name);
        }

        return pkgs;
    }
    std::vector<installed_package> installed_package::resolve(const std::vector<std::string>& args) {
        std::vector<installed_package> pkgs{};

        for (const auto& name : args) {
            auto result = parse_local(name);
            if (result.has_value()) {
                pkgs.push_back(std::move(result.value()));
            } else {
                raise("Invalid package: {}", name);
            }
        }

        return pkgs;
    }
    bool installed_package::is_installed(std::string_view name) {
        const auto path = fmt::format("{}/{}/package.info", pkgdir, name);
        return ::access(path.c_str(), F_OK) == 0;
    }

    std::list<std::string> installed_package::get_files(std::string_view name) {
        const auto path = fmt::format("{}/{}/files", pkgdir, name);
        std::FILE* file = std::fopen(path.c_str(), "r");
        if (!file)
            raise("Failed to open '{}'.", path);

        std::list<std::string> files{};
        char buffer[PATH_MAX + 10];
        while (std::fgets(buffer, sizeof buffer, file) != nullptr) {
            std::string str = buffer;
            if (str.length() >= PATH_MAX) {
                printerr(color::WARN, "Path '{}' is longer than PATH_MAX.", str);
            }
            files.push_back(std::move(str.substr(0, str.size()-1)));
        }

        std::fclose(file);
        return files;
    }

    // build()
    std::optional<binary_package> source_package::build(std::string_view path_binpkg, const std::string& filesdir) const {
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
        ::posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
        ::posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
        ::posix_spawn_file_actions_addclose(&actions, pipefd[0]);
        ::posix_spawn_file_actions_addclose(&actions, pipefd[1]);

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
        env.push_back(nullptr);

        ::pid_t pid;
        if (::posix_spawnp(&pid, "bash", &actions, nullptr, args.data(), env.data()) != 0)
            raise("{}: build(): Failed to posix_spawn() the shell.", name);

        // Cleanup.
        xclose(pipefd[1]);
        free_environ(args);
        free_environ(env);
        ::posix_spawn_file_actions_destroy(&actions);

        std::FILE* log = ::fdopen(pipefd[0], "r");
        assert(log != nullptr);

        std::FILE* logfile = std::fopen(path_logfile.c_str(), "w");
        if (!logfile) {
            printerr(color::WARN, "{}: Failed to open log file '{}'.", name, path_logfile);
        }

        char buffer[100];
        while (std::fgets(buffer, sizeof buffer, log) != nullptr) {
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

        binary_package_info info(*this, std::time(nullptr));

        mkdir_p(path_metadir);
        write_file(path_metadir + "/package.info", info.to_file());

        const auto cmd = fmt::format("cd '{}' && fakeroot tar -caf '{}' .", path_pkgdir, path_binpkg);
        if (std::system(cmd.c_str()) != 0) {
            printerr(color::ERROR, "Can't create binary package.");
            return {};
        }
        return binary_package{ std::string(path_binpkg), std::move(info) };
    }

    // install()
    bool binary_package::install() const {
        const auto pkg_pkgdir       = fmt::format("{}/{}", pkgdir, pkg.name);
        const auto pkg_filesfile    = pkg_pkgdir + "/files";
        std::string cmd;

        // TODO: Check for superuser priviliges.

        mkdir_p(pkg_pkgdir);

        std::list<std::string> old_files;
        if (installed_package::is_installed(pkg.name))
            old_files = installed_package::get_files(pkg.name);

        const auto get_new_files = [this] {
            const auto cmd = fmt::format("tar -tf '{}' --exclude='.meta'", path);
            std::FILE* file = ::popen(cmd.c_str(), "r");
            if (!file)
                raise("Can't get list of files of '{}'.", path);

            std::vector<std::string> files{};
            while (!std::feof(file)) {
                const auto line = freadline(file);
                if (line.empty())
                    break;
                files.push_back(line.substr(1));
            }
            ::pclose(file);
            return files;
        };

        const auto new_files = get_new_files();

        // Extract the package.
        const auto opts = verbosity >= verbosity_level::VERBOSE ? "-xhpvf" : "-xhpf";
        cmd = fmt::format("tar -C '{}' {} '{}' --exclude='.meta'", rootdir, opts, path);
        if (std::system(cmd.c_str()) != 0) {
            printerr(color::ERROR, "{}: Failed to extract package.", pkg.name);
            return false;
        }

        // Write new_files into files file.
        std::FILE* files_file = std::fopen(pkg_filesfile.c_str(), "w");
        if (!files_file) {
            printerr(color::ERROR, "Failed to open '{}'.", pkg_filesfile);
            return false;
        }

        for (const auto& f : new_files) {
            std::fprintf(files_file, "%s\n", f.c_str());
        }

        std::fclose(files_file);


        // Find and delete files that are part of the old package
        // but not in the new package...
        if (!old_files.empty()) {
            for (const auto& f : new_files) {
                old_files.remove(f);
            }
            rm(rootdir, old_files);
        }

        // Create symlinks to provided packages.
        for (const auto& name : pkg.provides) {
            const auto link = fmt::format("{}/{}", pkgdir, name);
            symlink_v(pkg.name, link);
        }

        // Create the package.info file.
        installed_package ipkg(pkg, std::time(nullptr));
        write_file(pkg_pkgdir + "/package.info", ipkg.to_file());

        // Run the post-install script, if available.
        cmd = fmt::format("tar -tf '{}' .meta/post-install.sh >/dev/null 2>/dev/null", path);
        if (std::system(cmd.c_str()) == 0) {
            cmd = fmt::format("tar -xf '{}' .meta/post-install -O | bash", path);
            if (std::system(cmd.c_str()) != 0) {
                printerr(color::WARN, "{}: Post-install script failed.", pkg.name);
                return false;
            }
        }

        // TODO: Copy the binpkg to /var/cache/minipkg2/binpkgs
        const auto binpkgsdir = cachedir + "/binpkgs";
        mkdir_p(binpkgsdir);
        cp(path, fmt::format("{}/{}", binpkgsdir, path.substr(path.rfind('/') + 1)));

        return true;
    }
    bool installed_package::uninstall() const {
        auto files = get_files();

        bool success = true;
        success &= rm(rootdir, files);

        // Remove the package and it's provided symlinks.
        for (const auto& p : provides) {
            success &= rm(fmt::format("{}/{}", pkgdir, p));
        }
        success &= rm_rf(fmt::format("{}/{}", pkgdir, name));

        return success;
    }
    std::size_t installed_package::estimate_size(std::string_view name) {
        const auto files = get_files(name);
        std::size_t size = 0;
        printerr(color::DEBUG, "Estimating size of {}...", name);
        for (const auto& f : files) {
            const auto path = fmt::format("{}/{}", rootdir, f);
            printerr(color::DEBUG, "{}", path);
            struct ::stat st;
            if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
                continue;
            size += st.st_size;
        }
        return size;
    }
    std::size_t installed_package::estimate_size(const std::vector<std::string>& names) {
        std::size_t size = 0;
        for (const auto& name : names) {
            size += estimate_size(name);
        }
        return size;
    }
}
