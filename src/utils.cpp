#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <climits>
#include <cstdio>
#include "utils.hpp"

namespace minipkg2 {
    std::string xreadlink(const std::string& filename) {
        char buf[PATH_MAX + 1];
        if (::readlink(filename.c_str(), buf, PATH_MAX) < 0)
            throw std::runtime_error("Failed to readlink(" + filename + ')');

        return buf;
    }
    std::pair<int, std::string> xpread(const std::string& cmd) {
        FILE* file = ::popen(cmd.c_str(), "r");
        if (!file)
            throw std::runtime_error("popen('" + cmd + "') failed.");

        std::string reply{};

        char buffer[128];
        while (std::fgets(buffer, sizeof buffer, file) != nullptr)
            reply += buffer;

        // Remove a possible trailing '\n'
        if (reply.length() != 0 && reply.back() == '\n')
            reply.erase(reply.end() - 1);

        return {::pclose(file), reply};
    }


    bool starts_with(std::string_view str, std::string_view prefix) {
        return str.substr(0, prefix.length()) == prefix;
    }
    bool ends_with(std::string_view str, std::string_view suffix) {
        return str.length() >= suffix.length() && str.substr(str.length() - suffix.length()) == suffix;
    }
    bool rm(const std::string& path) {
        // TODO: Add logging.
        return std::remove(path.c_str()) == 0;
    }
    bool rm_rf(const std::string& path) {
        struct stat st;
        if (::lstat(path.c_str(), &st) != 0)
            return false;

        bool success = true;
        if ((st.st_mode & S_IFMT) == S_IFDIR) {
            DIR* dir = ::opendir(path.c_str());
            if (!dir)
                return false;

            struct dirent* ent;
            while ((ent = ::readdir(dir)) != NULL) {
                if (!std::strcmp(ent->d_name, ".") || !std::strcmp(ent->d_name, ".."))
                    continue;

                success &= rm_rf(path + '/' + ent->d_name);
            }
            ::closedir(dir);
        }
        return success & rm(path);
    }

    std::string fix_path(const std::string& path) {
        if (path[0] == '/') {
            return path;
        } else {
            return CONFIG_PREFIX "/" + path;
        }
    }
}
