#ifndef FILE_MINIPKG2_PRINT_HPP
#define FILE_MINIPKG2_PRINT_HPP
#include <iostream>

namespace minipkg2 {
    enum class Color : unsigned {
        DEBUG   = 93,
        LOG     = 32,
        INFO    = 34,
        WARN    = 33,
        ERROR   = 31,
    };


    template<Color color>
    inline std::ostream& star(std::ostream& stream) {
        return stream << "\033[" << static_cast<unsigned>(color) << "m * \033[0m";
    }
}

#endif /* FILE_MINIPKG2_PRINT_HPP */
