#include <fmt/format.h>
#include <cstdio>
#include "minipkg2.hpp"
#include "quickdb.hpp"
#include "utils.hpp"
#include "print.hpp"

namespace minipkg2::quickdb {
    quickdb read(std::string_view name) {
        const auto filename = fmt::format("{}/{}.db", dbdir, name);
        std::FILE* file = std::fopen(filename.c_str(), "r");
        if (!file)
            return {};
        quickdb db{};

        std::string line;
        while (freadline(file, line)) {
            const auto end_name = line.find(':');
            if (end_name == std::string::npos) {
                printerr(color::WARN, "Invalid conflicts.db file.");
                continue;
            }
            const auto name = line.substr(0, end_name);
            std::set<std::string> conflicts{};

            auto it = line.begin() + end_name + 1;
            if (it != line.end()) {
                while (true) {
                    const auto start = it;
                    while (it != line.end() && *it != ',')
                        ++it;
                    conflicts.emplace(start, it);
                    if (it == line.end() || *it != ',')
                        break;
                    ++it;
                }
            }
            db[name] = conflicts;
        }

        std::fclose(file);
        return db;
    }
    void write(std::string_view name, const quickdb& db) {
        const auto filename = fmt::format("{}/{}.db", dbdir, name);
        std::FILE* file = std::fopen(filename.c_str(), "w");

        for (const auto& e : db) {
            fmt::print(file, "{}:", e.first);
            const auto& snd = e.second;
            if (!snd.empty()) {
                auto it = snd.begin();
                fmt::print(file, "{}", *it);
                for (++it; it != snd.end(); ++it)
                    fmt::print(file, ",{}", *it);
            }
            fmt::print(file, "\n");
        }

        std::fclose(file);
    }
    void set(std::string_view dbname, const std::string& name, const std::set<std::string>& value) {
        auto db = read(dbname);
        db[name] = value;
        write(dbname, db);
    }
    void set(std::string_view dbname, const std::string& name, std::set<std::string>&& value) {
        auto db = read(dbname);
        db[name] = std::move(value);
        write(dbname, db);
    }
    void remove(std::string_view dbname, const std::string& name) {
        auto db = read(dbname);
        db.erase(name);
        write(dbname, db);
    }
}
