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
    std::string dbdir{};
    std::string pkgdir{};
    std::string builddir{};
    std::string repodir{};
    std::string cachedir{};
    std::string host{};
    std::size_t jobs{};
    std::string self{};
    miniconf::miniconf config{};

    void set_root(std::string_view root) {
        rootdir         = root;
        dbdir           = rootdir   + "/var/db/minipkg2";
        pkgdir          = dbdir     + "/packages";
        repodir         = dbdir     + "/repo";
        builddir        = rootdir   + "/var/tmp/minipkg2";
        cachedir        = rootdir   + "/var/cache/minipkg2";
    }


    static bool load_config() {
        std::ifstream file(config_filename);
        if (!file) {
            printerr(color::WARN, "Config file '{}' doesn't exist.", config_filename);
            return true;
        }

        auto result = miniconf::parse(file);

        if (auto* errmsg = std::get_if<std::string>(&result)) {
            printerr(color::ERROR, "Failed to load config '{}': {}", config_filename, *errmsg);
            return false;
        }

        config = std::get<miniconf::miniconf>(result);
        return true;
    }


    bool init_self() {
        try {
            self = xreadlink("/proc/self/exe");
        } catch (const std::runtime_error& e) {
            printerr(color::ERROR, "{}", e.what());
            return false;
        }

        set_root("/");

        return load_config();
    }
    void print_version(void) {
        fmt::print("Micro-Linux Package Manager\n\n"
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
                   "\nWritten by Benjamin Stürz <benni@stuerz.xyz>.\n");
    }
}
