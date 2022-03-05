#ifndef FILE_MINIPKG2_BASHCONFIG_HPP
#define FILE_MINIPKG2_BASHCONFIG_HPP
#include <stdexcept>
#include <variant>
#include <string>
#include <vector>
#include <cstdio>
#include <map>

namespace minipkg2::bashconfig {
    struct parse_error : std::runtime_error {
        parse_error(const std::string& str) : std::runtime_error(str) {}
        parse_error(const parse_error&) = default;
        parse_error(parse_error&&) = default;
        ~parse_error() = default;
    };
    using value = std::variant<std::string, std::vector<std::string>>;
    using config = std::map<std::string, value>;

    config read(std::FILE* file);
    void write(std::FILE* file, const config& conf);
}

#endif /* FILE_MINIPKG2_BASHCONFIG_HPP */
