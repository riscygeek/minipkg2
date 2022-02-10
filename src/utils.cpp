#include <stdexcept>
#include <unistd.h>
#include <limits.h>
#include "utils.hpp"

namespace minipkg2 {
    std::string xreadlink(const std::string& filename) {
        char buf[PATH_MAX + 1];
        if (readlink(filename.c_str(), buf, PATH_MAX) < 0)
            throw std::runtime_error("Failed to readlink(" + filename + ')');

        return buf;
    }
}
