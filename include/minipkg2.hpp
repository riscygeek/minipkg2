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

#define CONFIG_SHELL    "/bin/bash"

#define fix_path(prefix, subdir, suffix) (subdir[0] == '/' ? (subdir suffix) : (prefix "/" subdir suffix))

namespace minipkg2 {
    extern std::string rootdir;
    extern std::string pkgdir;
    extern std::string builddir;
    extern std::string repodir;
    extern std::string cachedir;
    extern std::string host;
    extern std::size_t jobs;
    extern std::string self;
    extern miniconf::miniconf config;

    constexpr auto config_filename  = fix_path(CONFIG_PREFIX, CONFIG_SYSCONFDIR, "/minipkg2.conf");
    constexpr auto env_filename     = fix_path(CONFIG_PREFIX, CONFIG_LIBDIR, "/minipkg2/env.bash");
    constexpr auto parse_filename   = fix_path(CONFIG_PREFIX, CONFIG_LIBDIR, "/minipkg2/parse.bash");
    constexpr auto build_filename   = fix_path(CONFIG_PREFIX, CONFIG_LIBDIR, "/minipkg2/build.bash");

    void set_root(std::string_view);
    bool init_self();
    void print_version();
}

#endif /* FILE_MINIPKG2_MINIPKG2_HPP */
