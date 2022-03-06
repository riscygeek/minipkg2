#include <sys/types.h>
#include <sys/stat.h>
#include <fmt/core.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <spawn.h>
#include <cassert>
#include <climits>
#include <map>
#include "minipkg2.hpp"
#include "package.hpp"
#include "quickdb.hpp"
#include "utils.hpp"
#include "print.hpp"
#include "git.hpp"

#if HAS_FAKEROOT
# define FAKEROOT "fakeroot"
#else
# define FAKEROOT ""
#endif

namespace minipkg2 {
    using namespace std::literals;
    template<class T>
    void assert_result(const std::optional<T>& opt, std::string_view name) {
        if (!opt.has_value()) {
            raise("Invalid package: {}", name);
        }
    }
    // print()

    void print_line(std::string_view name, std::string_view value) {
        fmt::print("{:30}: {}\n", name, value);
    }
    template<class It>
    void print_line(std::string_view name, const It begin, const It end) {
        fmt::print("{:30}:", name);
        for (auto it = begin; it != end; ++it)
            fmt::print(" {}", *it);
        fmt::print("\n");
    }
    void package_base::print() const {
        print_line("Name",                  name);
        print_line("Version",               version);
        print_line("URL",                   url);
        print_line("Description",           description);
        print_line("Runtime Dependencis",   begin(rdepends), end(rdepends));
        print_line("Provides",              begin(provides), end(provides));
        print_line("Conflicts",             begin(conflicts), end(conflicts));
    }
    void source_package::print() const {
        package_base::print();
        print_line("Sources",               begin(sources), end(sources));
        print_line("Build Dependencies",    begin(bdepends), end(bdepends));
        print_line("Features",              begin(features), end(features));
    }
    void binary_package_info::print() const {
        package_base::print();
        const auto bdate = uts_to_str(build_date);
        print_line("Build Date",            bdate);
    }
    void installed_package::print() const {
        binary_package_info::print();
        const auto idate = uts_to_str(install_date);
        print_line("Install Date",          idate);
    }


    // to_config()
    bashconfig::config binary_package_info::to_config() const {
        bashconfig::config conf{};
        conf["pkgname"]     = name;
        conf["pkgver"]      = version;
        conf["url"]         = url;
        conf["description"] = description;
        conf["rdepends"]    = rdepends;
        conf["provides"]    = std::vector<std::string>(begin(provides), end(provides));
        conf["conflicts"]   = std::vector<std::string>(begin(conflicts), end(conflicts));
        conf["build_date"]  = std::to_string(build_date);
        return conf;
    }

    bashconfig::config installed_package::to_config() const {
        auto conf = binary_package_info::to_config();
        conf["install_date"] = std::to_string(install_date);
        return conf;
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
        std::set<std::string> provides;
        std::set<std::string> conflicts;
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

        const auto read_set = [file] {
            std::set<std::string> lines{};
            std::string line;
            while (freadline(file, line) && line != "--"sv) {
                lines.insert(std::move(line));
            }
            return lines;
        };
        const auto read_vec = [file] {
            std::vector<std::string> lines{};
            std::string line;
            while (freadline(file, line) && line != "--"sv) {
                lines.push_back(std::move(line));
            }
            return lines;
        };

        // Parse the output.
        generic_package pkg;
        pkg.filename        = filename;
        freadline(file, pkg.name);
        freadline(file, pkg.version);
        freadline(file, pkg.url);
        freadline(file, pkg.description);
        pkg.sources         = read_vec();
        pkg.bdepends        = read_vec();
        pkg.rdepends        = read_vec();
        pkg.provides        = read_set();
        pkg.conflicts       = read_set();
        pkg.features        = read_vec();
        std::string tmp;
        freadline(file, tmp);
        pkg.build_date      = str_to_uts(tmp);
        freadline(file, tmp);
        pkg.install_date    = str_to_uts(tmp);

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
    std::vector<install_transaction> source_package::resolve_conflicts(const std::vector<source_package>& pkgs, bool strict) {
        const auto cdb = quickdb::read("conflicts");
        std::vector<install_transaction> transactions{};
        bool success = true;

        for (const auto& pkg : pkgs) {
            install_transaction trans;
            trans.pkg = &pkg;

            // 1. Check if it conflicts with a selected package.
            for (const auto& p2 : pkgs) {
                if (&pkg == &p2)
                    continue;

                for (const auto& c : pkg.conflicts) {
                    if (p2.is_provider_of(c)) {
                        printerr(color::ERROR, "Package '{:v}' conflicts with '{:v}'.", pkg, p2);
                        success = false;
                        break;
                    }
                }
            }

            if (!success)
                return {};

            // 2. Check if local packages conflict with the selected one.
            for (const auto& [name, conflicts] : cdb) {
                if (pkg.is_provider_of(name)) {
                    if (name != pkg.name) {
                        auto result = installed_package::parse_local(name);
                        assert_result(result, name);
                        printerr(color::WARN, "Package '{:v}' will be removed because it conflicts with '{:v}'.", result.value(), pkg);
                        trans.remove.push_back(std::move(result.value()));
                    }
                    continue;
                }

                for (const auto& c : conflicts) {
                    if (c == pkg.name) {
                        if (pkg.is_provider_of(c) && !strict) {
                            auto result = installed_package::parse_local(name);
                            assert_result(result, name);
                            printerr(color::WARN, "Package '{:v}' will be removed because it conflicts with '{:v}'.", result.value(), pkg);
                            trans.remove.push_back(std::move(result.value()));
                        } else {
                            printerr(color::ERROR, "Package '{}' conflicts with '{:v}'.", name, pkg);
                            success = false;
                        }
                        break;
                    }
                }
            }

            transactions.push_back(std::move(trans));
        }

        return success ? transactions : std::vector<install_transaction>{};
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
            return (skip_installed && installed_package::is_installed(name))
                || contains_if(pkgs, [name](const auto& pkg) {
                    return pkg.is_provider_of(name);
                });
        };

        std::function<void(const std::vector<std::string>&)> do_resolve;

        const auto add = [&](std::string_view name) {
            auto result = source_package::parse_repo(name);
            assert_result(result, name);
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
            assert_result(result, name);
            pkgs.push_back(std::move(result.value()));
        }

        return pkgs;
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
        bashconfig::write_file(path_metadir + "/package.info", info.to_config());

        // Find all files
        std::string files_list = ".meta";
        ::DIR* dir = ::opendir(path_pkgdir.c_str());
        assert(dir != nullptr);
        struct ::dirent* ent;

        while ((ent = ::readdir(dir)) != nullptr) {
            if (ent->d_name[0] == '.')
                continue;
            files_list += fmt::format(" '{}'", ent->d_name);
        }

        ::closedir(dir);

        const auto cmd = fmt::format("{} tar -C '{}' -caf '{}' {}", FAKEROOT, path_pkgdir, path_binpkg, files_list);
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
        auto old_pkg = installed_package::parse_local(pkg.name);
        if (old_pkg.has_value()) {
            old_files = installed_package::get_files(pkg.name);
        }

        const auto get_new_files = [this] {
            const auto cmd = fmt::format("tar -tf '{}' --exclude='.meta'", path);
            std::FILE* file = ::popen(cmd.c_str(), "r");
            if (!file)
                raise("Can't get list of files of '{}'.", path);

            std::vector<std::string> files{};
            std::string line;
            while (freadline(file, line)) {
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

        // Remove old symlinks, if any.
        if (old_pkg.has_value()) {
            for (const auto& name : old_pkg.value().provides) {
                rm(fmt::format("{}/{}", pkgdir, name));
            }
        }

        // Create symlinks to provided packages.
        for (const auto& name : pkg.provides) {
            symlink_v(pkg.name, fmt::format("{}/{}", pkgdir, name));
        }

        // Create the package.info file.
        installed_package ipkg(pkg, std::time(nullptr));
        bashconfig::write_file(pkg_pkgdir + "/package.info", ipkg.to_config());

        // Run the post-install script, if available.
        cmd = fmt::format("tar -tf '{}' .meta/post-install.sh >/dev/null 2>/dev/null", path);
        if (std::system(cmd.c_str()) == 0) {
            cmd = fmt::format("tar -xf '{}' .meta/post-install.sh -O | bash", path);
            if (std::system(cmd.c_str()) != 0) {
                printerr(color::WARN, "{}: Post-install script failed.", pkg.name);
                return false;
            }
        }

        quickdb::set("conflicts", pkg.name, pkg.conflicts);

        // Configure the reverse-dependencies.
        auto db = quickdb::read("rdeps");
        db[pkg.name] = {};
        for (const auto& dep : pkg.rdepends) {
            db[dep].insert(pkg.name);
        }
        quickdb::write("rdeps", db);

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

        if (!success)
            return false;

        // Remove package from conflicts.db
        quickdb::remove("conflicts", name);

        // Remove package and references to it from rdeps.db
        auto db = quickdb::read("rdeps");
        db.erase(name);
        for (auto& [_, rdeps] : db) {
            rdeps.erase(name);
        }
        quickdb::write("rdeps", db);
        quickdb::remove("rdeps", name);

        return true;
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
    std::optional<binary_package> binary_package::load(std::string path) {
        auto result = binary_package_info::parse_file(path);
        return result.has_value() ? binary_package{ std::move(path), std::move(result.value()) } : std::optional<binary_package>{};
    }
    bool installed_package::is_installed(std::string_view name) {
         const auto path = fmt::format("{}/{}/package.info", pkgdir, name);
         return ::access(path.c_str(), F_OK) == 0;
    }
}
