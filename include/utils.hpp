#ifndef FILE_MINIPKG2_UTILS_HPP
#define FILE_MINIPKG2_UTILS_HPP
#include <sys/types.h>
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
    bool download(const std::string& url, const std::string& dest, bool overwrite);
    bool mkparentdirs(std::string dir, mode_t mode);
}

#endif /* FILE_MINIPKG2_UTILS_HPP */
