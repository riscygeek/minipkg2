#ifndef FILE_MINIPKG2_UTILS_HPP
#define FILE_MINIPKG2_UTILS_HPP
#include <string_view>
#include <utility>
#include <string>
#include <cstdio>

namespace minipkg2 {
    std::string xreadlink(const std::string& filename);
    std::pair<int, std::string> xpread(const std::string& cmd);
    std::string freadline(FILE* file);

    bool starts_with(std::string_view str, std::string_view prefix);
    bool ends_with(std::string_view str, std::string_view suffix);
    bool rm_rf(const std::string& path);
}

#endif /* FILE_MINIPKG2_UTILS_HPP */
