#ifndef FILE_MINIPKG2_MINIPKG2_HPP
#define FILE_MINIPKG2_MINIPKG2_HPP
#include <string_view>
#include <string>
#include "miniconf.hpp"

#ifndef VERSION
#error "VERSION is not defined"
#endif

#ifndef CONFIG_PREFIX
#error "CONFIG_PREFIX is not defined"
#endif

#ifndef CONFIG_SYSCONFDIR
#error "CONFIG_SYSCONFDIR is not defined"
#endif

#ifndef CONFIG_LIBDIR
#error "CONFIG_LIBDIR is not defined"
#endif

#define CONFIG_LOG_DIR  CONFIG_PREFIX "/var/log"
#define CONFIG_LOG_FILE CONFIG_LOG_DIR "/minipkg2.log"

#define CONFIG_SHELL    "bash"

namespace minipkg2 {
    extern std::string rootdir;
    extern std::string pkgdir;
    extern std::string builddir;
    extern std::string repodir;
    extern std::string host;
    extern std::size_t jobs;
    extern std::string self;
    extern std::string env_file;
    extern miniconf::miniconf config;

    void set_root(std::string_view);
    bool init_self();
    void print_version();
}

#endif /* FILE_MINIPKG2_MINIPKG2_HPP */
