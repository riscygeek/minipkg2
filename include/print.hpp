#ifndef FILE_MINIPKG2_PRINT_HPP
#define FILE_MINIPKG2_PRINT_HPP
#include <fmt/core.h>

namespace minipkg2 {
    enum class color : unsigned {
        //NONE    = 0,
        DEBUG   = 93,
        LOG     = 32,
        INFO    = 34,
        WARN    = 33,
        ERROR   = 31,
    };

    enum class verbosity_level : unsigned {
        QUIET,
        NORMAL,
        VERBOSE,
    };

    extern verbosity_level verbosity;

    inline bool check_verbosity(color c) {
        switch (c) {
        case color::ERROR:
            return true;
        case color::WARN:
        case color::INFO:
        case color::LOG:
            return verbosity >= verbosity_level::NORMAL;
        case color::DEBUG:
            return verbosity >= verbosity_level::VERBOSE;
        default:
            return false;
        }
    }

    template<class... Args>
    inline void printerr(color c, Args&&... args) {
        if (!check_verbosity(c))
            return;
        fmt::print("\033[{}m * \033[0m{}\n", static_cast<unsigned>(c), fmt::format(args...));
    }
    template<class... Args>
    inline void print(color c, Args&&... args) {
        if (!check_verbosity(c))
            return;
        fmt::print("\033[{}m * \033[0m{}", static_cast<unsigned>(c), fmt::format(args...));
    }
}

#endif /* FILE_MINIPKG2_PRINT_HPP */
