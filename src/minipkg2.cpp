#include <iostream>
#include "minipkg2.hpp"
#include "utils.hpp"
#include "print.hpp"

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
    miniconf::miniconf config{};

    void set_root(std::string_view root) {
        rootdir     = root;
        pkgdir      = rootdir + "/var/db/minipkg2/packages";
        repodir     = rootdir + "/var/db/minipkg2/repo";
        builddir    = rootdir + "/var/tmp/minipkg2";
    }


    static bool load_config() {
        std::ifstream file(config_filename);
        if (!file) {
            std::cerr << star<color::WARN> << "Config file '" << config_filename << "' doesn't exist.\n";
            return true;
        }

        auto result = miniconf::parse(file);

        if (auto* errmsg = std::get_if<std::string>(&result)) {
            std::cerr << star<color::ERROR> << "Failed to load config: " << *errmsg << '\n';
            return false;
        }

        config = std::get<miniconf::miniconf>(result);
        return true;
    }


    bool init_self() {
        try {
            self = xreadlink("/proc/self/exe");
        } catch (const std::runtime_error& e) {
            std::cerr << star<color::ERROR> << e.what() << '\n';
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
