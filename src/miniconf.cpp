#include <cctype>
#include "miniconf.hpp"

namespace minipkg2::miniconf {
    std::variant<std::string, miniconf> parse(std::istream& file) {
        miniconf conf{};
        std::size_t linenum{};

        // Read a line from file and respect trailing '\'
        const auto readline = [&](std::string& line) -> bool {
            std::string tmp{};
            line.clear();
            do {
                if (!std::getline(file, tmp))
                    return false;
                line += tmp;
                ++linenum;
            } while (!line.empty() && line.back() == '\\' && (line.erase(line.end() - 1), true));
            return true;
        };

        const auto isvalidname = [](char ch) { return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.'; };
        const auto errbeg = [&linenum] { return std::to_string(linenum) + ": "; };
        const auto skip = [](std::string_view& view) { view = view.substr(1); };
        const auto skip_ws = [skip](std::string_view& view) {
            while (!view.empty() && std::isspace(view.front()))
                skip(view);
        };

        std::string cursection{};
        std::string line{};
        while (readline(line)) {
            std::string_view view = line;
            skip_ws(view);
            if (view.empty() || view.front() == '#')
                continue;

            if (view.front() == '[') {
                // Parse sections.
                cursection.clear();
                skip(view);
                while (!view.empty() && view.front() != ']') {
                    const char ch = view.front();
                    if (!isvalidname(ch))
                        return errbeg() + "Invalid character for section name: '" + ch + '\'';


                    cursection += view.front();
                    skip(view);
                }

                if (view.empty()) {
                    return errbeg() + "Unexpected end of file.";
                }

                skip(view);
                skip_ws(view);

                if (view.empty() || view.front() == '#')
                    continue;

                return errbeg() + "Garbage at the end of the section definition.";
            } else {
                // Parse settings.
                std::string name{};

                while (!view.empty() && isvalidname(view.front())) {
                    name += view.front();
                    skip(view);
                }

                if (name.empty())
                    return errbeg() + "Expected name.";

                skip_ws(view);

                if (view.empty() || view.front() != '=')
                    return errbeg() + "Expected '=' after name.";

                skip(view);
                skip_ws(view);

                conf[cursection + "." + name] = view;
            }
        }

        return conf;
    }
    void dump(std::ostream& stream, const miniconf& conf) {
        for (const auto& e : conf) {
            stream << "config[" << e.first << "]='" << e.second << "'\n";
        }
    }
}
