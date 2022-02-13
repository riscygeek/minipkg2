#include <iostream>
#include "minipkg2.hpp"
#include "utils.hpp"

#if HAS_LIBCURL
# define LIBCURL_TF "true"
#else
# define LIBCURL_TF "false"
#endif

namespace minipkg2 {
    std::string rootdir{};
    std::string pkgdir{};
    std::string builddir{};
    std::string repodir{};
    std::string host{};
    std::size_t jobs{};
    std::string self{};
    std::string env_file = fix_path(CONFIG_LIBDIR "/minipkg2/env.bash");
    miniconf::miniconf config{};

    void set_root(std::string_view root) {
        rootdir     = root;
        pkgdir      = rootdir + "/var/db/minipkg2/packages";
        repodir     = rootdir + "/var/db/minipkg2/repo";
        builddir    = rootdir + "/var/tmp/minipkg2";
    }


    static bool load_config() {
        const std::string path = fix_path(CONFIG_SYSCONFDIR "/minipkg2.conf");
        std::ifstream file(path);
        if (!file) {
            std::cerr << "Failed to open config file '" << path << "'.\n";
            return true;
        }

        auto result = miniconf::parse(file);

        if (auto* errmsg = std::get_if<std::string>(&result)) {
            std::cerr << "Failed to load config: " << *errmsg << '\n';
            return false;
        }

        config = std::get<miniconf::miniconf>(result);
        return true;
    }


    bool init_self() {
        try {
            self = xreadlink("/proc/self/exe");
        } catch (const std::runtime_error& e) {
            std::cerr << e.what() << '\n';
            return false;
        }

        set_root("/");

        return load_config();
    }
    void print_version(void) {
        std::cout << "Micro-Linux Package Manager\n\n"
                     "Version: " VERSION "\n"
                     "\nBuild:\n"
                     "  system:     " BUILD_SYS "\n"
                     "  prefix:     " CONFIG_PREFIX "\n"
                     "  libdir:     " CONFIG_PREFIX "/" CONFIG_LIBDIR "\n"
                     "  sysconfdir: " CONFIG_PREFIX "/" CONFIG_SYSCONFDIR "\n"
                     "  date:       " __DATE__ "\n"
                     "  time:       " __TIME__ "\n"
                     "\nFeatures:\n"
                     "  libcurl:    " LIBCURL_TF "\n"
                     "\nWritten by Benjamin Stürz <benni@stuerz.xyz>.\n";
    }
}
