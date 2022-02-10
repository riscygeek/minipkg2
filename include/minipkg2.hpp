#ifndef FILE_MINIPKG2_MINIPKG2_HPP
#define FILE_MINIPKG2_MINIPKG2_HPP
#include <string_view>
#include <string>

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

#define CONFIG_FILE     CONFIG_PREFIX "/" CONFIG_SYSCONFDIR "/minipkg2.conf"
#define ENV_FILE        CONFIG_PREFIX "/" CONFIG_LIBDIR     "/minipkg2/env.bash"

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

    void set_root(std::string_view);
    void init_self();
    void print_version();
}

#endif /* FILE_MINIPKG2_MINIPKG2_HPP */
