#include <cctype>
#include "bashconfig.hpp"
#include "utils.hpp"

namespace minipkg2::bashconfig {
    [[nodiscard]]
    static bool isname1(const int ch) noexcept {
        return std::isalpha(ch) || ch == '_';
    }
    [[nodiscard]]
    static bool isname(const int ch) noexcept {
        return std::isalnum(ch) || ch == '_';
    }

    config read(std::FILE* file) {
        int ch;
        config conf{};
        std::size_t lineNum = 0;
        std::function<int()> next;
        next = [file, &lineNum, next] {
            int ch = std::fgetc(file);
            if (ch == '\n') {
                ++lineNum;
            } else if (ch == '\\') {
                ch = std::fgetc(file);
                if (ch == '\n') {
                    ++lineNum;
                    return next();
                } else {
                    std::ungetc(ch, file);
                    return static_cast<int>('\\');
                }
            }
            return ch;
        };
        while (true) {
            ch = next();
            if (ch == EOF) {
                break;
            } else if (ch == '#') {
                while ((ch = next()) && ch != '\n' && ch != EOF);
                ++lineNum;
            } else if (std::isspace(ch)) {
                continue;
            } else if (isname1(ch)) {
                std::string name(1, ch);
                while (isname(ch = next())) {
                    name += ch;
                }

                if (ch != '=')
                    raise<parse_error>("{}: Expected '=', got '{}'.", lineNum, static_cast<char>(ch));

                ch = next();
                value val;
                switch (ch) {
                case '(': {
                    std::vector<std::string> tmp{};
                    while ((ch = next()) != ')' && ch != EOF) {
                        while (std::isspace(ch))
                            ch = next();
                        if (ch == ')')
                            break;
                        if (ch == '\'') {
                            std::string tmpv{};
                            while ((ch = next()) != '\'' && ch != '\n' && ch != EOF) {
                                tmpv.push_back(ch);
                            }
                            if (ch == '\n') {
                                raise<parse_error>("{}: Unexpected end of line.", lineNum);
                            } else if (ch == EOF) {
                                raise<parse_error>("{}: Unexpected end of file.", lineNum);
                            }
                            tmp.push_back(tmpv);
                        } else {
                            raise<parse_error>("{}: Expected ''', got '{}'.", lineNum, static_cast<char>(ch));
                        }
                    }
                    val = tmp;
                    break;
                }
                case '\'': {
                    std::string tmp{};
                    while ((ch = next()) != '\'' && ch != '\n' && ch != EOF) {
                        tmp += ch;
                    }
                    if (ch == '\n') {
                        raise<parse_error>("{}: Unexpected end of line.", lineNum);
                    } else if (ch == EOF) {
                        raise<parse_error>("{}: Unexpected end of file.", lineNum);
                    }
                    val = tmp;
                    break;
                }
                default:
                    raise<parse_error>("{}: Invalid character: '{}'", lineNum, static_cast<char>(ch));
                }
                while (std::isspace(ch = next()) && ch != '\n');
                if (ch != '\n' && ch != EOF)
                    raise<parse_error>("{}: Garbage at the end of line: '{}'.", lineNum, static_cast<char>(ch));
                conf[name]=val;
            } else {
                raise<parse_error>("{}: Invalid input: '{}'.", lineNum, static_cast<char>(ch));
            }
        }
        return conf;
    }
    void write(std::FILE* file, const config& conf) {
        const auto check_name = [](const std::string& name) {
            if (name.empty() || !isname1(name.front()))
                raise("Invalid name: '{}'", name);
            for (const char ch : name) {
                if (!isname(ch))
                    raise("Invalid name: '{}'", name);
            }
        };
        const auto check_value = [](const std::string& value) {
            if (contains(value, '\'') || contains(value, '\n'))
                raise("Invalid value: '{}'", value);
        };
        for (const auto& [name, value] : conf) {
            check_name(name);
            switch (value.index()) {
            case 0: {
                const auto& str = std::get<std::string>(value);
                check_value(str);
                fmt::print(file, "{}='{}'\n", name, str);
                break;
            }
            case 1: {
                const auto& vec = std::get<std::vector<std::string>>(value);
                if (vec.empty()) {
                    fmt::print(file, "{}=()\n", name);
                } else {
                    auto it = vec.begin();
                    fmt::print(file, "{}=('{}'", name, *it++);
                    for (; it != vec.end(); ++it) {
                        fmt::print(file, " '{}'", *it);
                    }
                    fmt::print(file, ")\n");
                }
                break;
            }
            default:
                raise("invalid value.");
            }
        }
    }
    bool write_file(const std::string& filename, const config& conf) {
        std::FILE* file = std::fopen(filename.c_str(), "w");
        if (!file)
            return false;
        write(file, conf);
        std::fclose(file);
        return true;
    }
}
