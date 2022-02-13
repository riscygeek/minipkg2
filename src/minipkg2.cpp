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

    void set_root(std::string_view root) {
        rootdir     = root;
        pkgdir      = rootdir + "/var/db/minipkg2/packages";
        repodir     = rootdir + "/var/db/minipkg2/repo";
        builddir    = rootdir + "/var/tmp/minipkg2";
    }
    void init_self() {
        self = xreadlink("/proc/self/exe");
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
