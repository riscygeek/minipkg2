#ifndef FILE_MINIPKG2_QUICKDB_HPP
#define FILE_MINIPKG2_QUICKDB_HPP
#include <string>
#include <set>
#include <map>

namespace minipkg2::quickdb {
    using quickdb = std::map<std::string, std::set<std::string>>;

    quickdb read(std::string_view name);
    void write(std::string_view name, const quickdb& db);

    void set(std::string_view dbname, const std::string& name, const std::set<std::string>& value);
    void set(std::string_view dbname, const std::string& name, std::set<std::string>&& value);
    void remove(std::string_view dbname, const std::string& name);
}

#endif /* FILE_MINIPKG2_QUICKDB_HPP */
