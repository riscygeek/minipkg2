#ifdef __linux__
#include <sys/sendfile.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdexcept>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <climits>
#include <fcntl.h>
#include <cstdio>
#include <array>
#include "utils.hpp"
#include "print.hpp"

extern "C" char** environ;

namespace minipkg2 {
    std::string xreadlink(const std::string& filename) {
        char buf[PATH_MAX + 1];
        const ssize_t n = ::readlink(filename.c_str(), buf, PATH_MAX);
        if (n < 0)
            raise("Failed to readlink({})", filename);
        buf[n] = '\0';
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
    std::string freadline(FILE* file) {
        std::string line{};
        while (true) {
            const int ch = std::fgetc(file);
            if (ch == EOF || ch == '\n')
                break;
            line.push_back(ch);
        }
        return line;
    }
    bool mkparentdirs(std::string dir, mode_t mode) {
        std::size_t pos = 1;
        while ((pos = dir.find('/', pos)) != std::string::npos) {
            dir[pos] = '\0';
            const int ec = ::mkdir(dir.c_str(), mode);
            if (ec != 0 && errno != EEXIST)
                return false;
            dir[pos] = '/';
            ++pos;
        }
        return true;
    }
    bool yesno(std::string_view question, bool defval) {
        print(color::INFO, "{} [{}] ", question, defval ? "Y/n" : "y/N");
        const char ch = fgetc(stdin);
        if (ch == '\n')
            return defval;
        char tmp;
        while ((tmp = fgetc(stdin)) != EOF && tmp != '\n');

        switch (ch) {
        case 'y':
        case 'Y':
            return true;
        case 'n':
        case 'N':
            return false;
        default:
            return yesno(question, defval);
        }
    }
    bool mkdir_p(const std::string& path, mode_t mode) {
        return mkparentdirs(path, mode) && (mkdir(path.c_str(), mode) == 0 || errno == EEXIST);
    }
    void xpipe(int pipefd[2]) {
        if (::pipe(pipefd) != 0)
            raise("Failed to create pipe.");
    }
    void xclose(int fd) {
        if (::close(fd) != 0)
            raise("Failed to close file descriptor {}.", fd);
    }
    char* xstrdup(std::string_view str) {
        char* new_str = new char[str.size() + 1];
        str.copy(new_str, std::string::npos);
        new_str[str.size()] = '\0';
        return new_str;
    }
    std::vector<char*> copy_environ() {
        std::vector<char*> env{};
        for (auto var = environ; *var != nullptr; ++var) {
            env.push_back(xstrdup(*var));
        }
        return env;
    }
    void add_environ(std::vector<char*>& env, const std::string& name, const std::string& value) {
        for (char* var : env) {
            char* eq = std::strchr(var, '=');
            if (name == std::string_view{var, static_cast<std::size_t>(eq - var)})
                return;
        }

        const std::size_t num = name.size() + value.size() + 2;
        char* var = new char[num];
        std::snprintf(var, num, "%s=%s", name.c_str(), value.c_str());
        env.push_back(var);
    }
    void free_environ(std::vector<char*>& env) {
        for (char* var : env) {
            delete[] var;
        }
        env.clear();
    }
    int xwait(pid_t pid) {
        int wstatus;
        if (::waitpid(pid, &wstatus, 0) != pid)
            raise("Failed to wait for process {}.", pid);
        if (!WIFEXITED(wstatus))
            raise("Process {} did not terminate.", pid);
        return WEXITSTATUS(wstatus);
    }
    void cat(FILE* out, const std::string& filename) {
        FILE* in = std::fopen(filename.c_str(), "r");
        if (!in)
            raise("Cannot open file '{}'.", filename);

        char buffer[100];
        while (std::fgets(buffer, sizeof buffer, in) != nullptr)
            std::fputs(buffer, out);

        std::fclose(in);
    }
    bool cp(const std::string& src, const std::string& dest) {
        const int out_fd = ::creat(dest.c_str(), 0644);
        if (out_fd < 0)
            return false;
        const int in_fd = ::open(src.c_str(), O_RDONLY);
        if (in_fd < 0) {
            xclose(out_fd);
            return false;
        }
        const auto close_all = [=] {
            xclose(out_fd);
            xclose(in_fd);
        };
        struct stat st_in;
        if (::fstat(in_fd, &st_in) != 0) {
            close_all();
            return false;
        }
#ifdef __linux__
        size_t count = static_cast<std::size_t>(st_in.st_size);
        off_t off{};
        while (count != 0) {
            const auto n = sendfile(out_fd, in_fd, &off, count);
            if (n < 0) {
                close_all();
                return false;
            }
            count -= n;
        }
#else
#error "cp() is only implemented on Linux."
#endif

        close_all();
        return true;
    }
    std::string uts_to_str(std::time_t time) {
        char buffer[100];
        struct tm* tmp = localtime(&time);
        if (strftime(buffer, sizeof buffer, "%c", tmp) == 0)
            raise("Failed to format time.");
        return buffer;
    }
    std::time_t str_to_uts(const std::string& str) {
        return str.empty() ? 0 : static_cast<std::time_t>(std::stoull(str));
    }
    bool write_file(const std::string& filename, std::string_view contents) {
        std::FILE* file = std::fopen(filename.c_str(), "w");
        if (!file)
            return false;

        std::fwrite(contents.data(), 1, contents.size(), file);

        std::fclose(file);
        return true;
    }
    bool rm(const std::string& file) {
        printerr(color::DEBUG, "rm -f '{}'", file);
        const bool success = remove(file.c_str()) == 0;
        if (!success && errno != ENOENT && errno != ENOTEMPTY) {
            printerr(color::WARN, "Failed to remove '{}': {}.", file, std::strerror(errno));
            return false;
        }
        return true;
    }
    bool rm(const std::string& prefix, std::list<std::string>& files) {
        while (!files.empty()) {
            bool success = false;
            for (auto it = files.begin(); it != files.end();) {
                const auto saved = it++;

                if (rm(prefix + *saved)) {
                    success = true;
                    files.erase(saved);
                }
            }
            if (!success)
                return false;
        }
        return true;
    }
    bool symlink_v(const std::string& dest, const std::string& file) {
        const bool success = symlink(dest.c_str(), file.c_str()) == 0;
        if (success) {
            printerr(color::DEBUG, "'{}' -> '{}'", file, dest);
        } else {
            printerr(color::WARN, "Failed to create symbolic link '{}' to '{}'.", file, dest);
        }
        return success;
    }
    std::string fmt_size(std::size_t sz) {
        struct unit {
            std::string_view name;
            std::size_t base;
        };
        static constexpr std::array units{
#if SIZE_MAX >= (1ull << 40)
            unit{"TiB", (std::size_t)1 << 40},
#endif
            unit{"GiB", 1 << 30},
            unit{"MiB", 1 << 20},
            unit{"KiB", 1 << 10},
        };

        for (const auto& u : units) {
            if (sz >= u.base) {
                return fmt::format("{:.2}{}", static_cast<double>(sz) / u.base, u.name);
            }
        }
        return fmt::format("{}B", sz);
    }
}
