#include <cstdio>
#include "minipkg2.hpp"
#include "utils.hpp"

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
       std::puts("Micro-Linux Package Manager\n");
       std::puts("Version: " VERSION);
       std::puts("\nBuild:");
       std::puts("  system:     " BUILD_SYS);
       std::puts("  prefix:     " CONFIG_PREFIX);
       std::puts("  libdir:     " CONFIG_PREFIX "/" CONFIG_LIBDIR);
       std::puts("  sysconfdir: " CONFIG_PREFIX "/" CONFIG_SYSCONFDIR);
       std::puts("  date:       " __DATE__);
       std::puts("  time:       " __TIME__);
       std::puts("\nFeatures:");
       std::printf("  libcurl:    %s\n", HAS_LIBCURL ? "true" : "false");
       std::puts("\nWritten by Benjamin Stürz <benni@stuerz.xyz>.");
    }
}
