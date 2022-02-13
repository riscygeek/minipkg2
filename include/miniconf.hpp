#ifndef FILE_MINIPKG2_MINICONF_HPP
#define FILE_MINIPKG2_MINICONF_HPP
#include <istream>
#include <ostream>
#include <variant>
#include <fstream>
#include <string>
#include <map>

namespace minipkg2::miniconf {
    using miniconf = std::map<std::string, std::string>;
    std::variant<std::string, miniconf> parse(std::istream& file);
    void dump(std::ostream&, const miniconf&);
}

#endif /* FILE_MINIPKG2_MINICONF_HPP */
