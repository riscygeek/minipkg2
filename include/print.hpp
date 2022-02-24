#ifndef FILE_MINIPKG2_PRINT_HPP
#define FILE_MINIPKG2_PRINT_HPP
#include <fmt/core.h>

namespace minipkg2 {
    enum class color : unsigned {
        NONE    = 0,
        DEBUG   = 93,
        LOG     = 32,
        INFO    = 34,
        WARN    = 33,
        ERROR   = 31,
    };


    template<class... Args>
    inline void printerr(color c, Args&&... args) {
        fmt::print(stderr, "\033[{}m * \033[0m{}\n", static_cast<unsigned>(c), fmt::format(args...));
    }
}

#endif /* FILE_MINIPKG2_PRINT_HPP */
