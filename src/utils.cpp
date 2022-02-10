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
    std::pair<int, std::string> xpread(const std::string& cmd) {
        FILE* file = popen(cmd.c_str(), "r");
        if (!file)
            throw std::runtime_error("popen('" + cmd + "') failed.");

        std::string reply{};

        char buffer[128];
        while (std::fgets(buffer, sizeof buffer, file) != nullptr)
            reply += buffer;

        return {pclose(file), reply};
    }


    bool starts_with(std::string_view str, std::string_view prefix) {
        return str.substr(0, prefix.length()) == prefix;
    }
    bool ends_with(std::string_view str, std::string_view suffix) {
        return str.length() >= suffix.length() && str.substr(str.length() - suffix.length()) == suffix;
    }
}
