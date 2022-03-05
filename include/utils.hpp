#ifndef FILE_MINIPKG2_UTILS_HPP
#define FILE_MINIPKG2_UTILS_HPP
#include <fmt/format.h>
#include <sys/types.h>
#include <string_view>
#include <algorithm>
#include <utility>
#include <string>
#include <vector>
#include <cstdio>
#include <ctime>
#include <list>

namespace minipkg2 {
    std::string xreadlink(const std::string& filename);
    std::pair<int, std::string> xpread(const std::string& cmd);
    bool freadline(FILE* file, std::string& line);
    std::string uts_to_str(std::time_t);
    std::time_t str_to_uts(const std::string&);
    char* xstrdup(std::string_view);
    int xwait(pid_t pid);
    std::string fmt_size(std::size_t);

    std::vector<char*> copy_environ();
    void add_environ(std::vector<char*>& environ, const std::string& name, const std::string& value);
    void free_environ(std::vector<char*>& environ);

    bool rm_rf(const std::string& path);
    bool mkdir_p(const std::string& path, mode_t mode = 0755);
    bool download(const std::string& url, const std::string& dest, bool overwrite = false);
    bool mkparentdirs(std::string dir, mode_t mode = 0755);
    bool yesno(std::string_view question, bool defval = true);
    void remap_null(int old_fd);
    void remap(int old_fd, int new_fd);
    void cat(FILE* out, const std::string& filename);
    bool cp(const std::string& src, const std::string& dest);
    bool write_file(const std::string& filename, std::string_view contents);
    bool rm(const std::string& file);
    bool rm(const std::string& prefix, std::list<std::string>& files);
    bool symlink_v(const std::string& dest, const std::string& file);

    // Error checking versions of common functions.
    void xpipe(int pipefd[2]);
    void xclose(int fd);

    template<class E = std::runtime_error, class... Args>
    [[noreturn]]
    inline void raise(Args&&... args) {
        throw E(fmt::format(std::forward<Args>(args)...));
    }

    // Functions for pre C++20
    bool starts_with(std::string_view str, std::string_view prefix);
    bool ends_with(std::string_view str, std::string_view suffix);
    template<class Container, class T>
    inline bool contains(const Container& container, const T& value) {
        return std::find(begin(container), end(container), value) != end(container);
    }

    template<class Container, class Pred>
    inline bool contains_if(const Container& container, const Pred& predicate) {
        return std::find_if(begin(container), end(container), predicate) != end(container);
    }
}

#endif /* FILE_MINIPKG2_UTILS_HPP */
