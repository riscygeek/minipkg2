#ifndef FILE_MINIPKG2_PACKAGE_HPP
#define FILE_MINIPKG2_PACKAGE_HPP
#include <fmt/format.h>
#include <string_view>
#include <algorithm>
#include <optional>
#include <numeric>
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <list>
#include <set>
#include <map>

namespace minipkg2 {
    struct package_base;
    struct source_package;
    struct binary_package;
    struct binary_package_info;
    struct installed_package;
    using conflictsdb = std::map<std::string, std::set<std::string>>;

    enum class resolve_skip_policy {
        NEVER,      // Never skip installed packages.
        DEPEND,     // Only skip installed dependencies.
        ALWAYS,     // Always skip installed packages.
    };

    struct package_base {
        std::string filename;
        std::string name;
        std::string version;
        std::string url;
        std::string description;
        std::string provided_by;
        std::vector<std::string> rdepends;
        std::set<std::string> provides;
        std::set<std::string> conflicts;

        virtual ~package_base() = default;
        virtual void print() const;

        bool is_provided() const noexcept;
        bool is_provider_of(std::string_view name) const noexcept;
        bool conflicts_with(std::string_view name) const noexcept;
        bool operator<(const package_base& other) const noexcept;
    };

    struct install_transaction {
        const source_package* pkg;
        std::vector<installed_package> remove;
    };
    struct source_package : package_base {
        std::vector<std::string> sources;
        std::vector<std::string> bdepends;
        std::vector<std::string> features;

        void print() const override;
        bool download() const;
        std::optional<binary_package> build(std::string_view path_binpkg, const std::string& filesdir) const;

        static bool                             download(const std::vector<source_package>&);
        static std::optional<source_package>    parse_file(const std::string& filename);
        static std::optional<source_package>    parse_repo(std::string_view name);
        static std::set<source_package>         parse_repo();
        static std::vector<source_package>      resolve(const std::vector<std::string>& args, bool resolve_deps, resolve_skip_policy policy);
        static std::vector<install_transaction> resolve_conflicts(const std::vector<source_package>& pkgs, bool strict = false);
    };

    struct binary_package_info : package_base {
        std::time_t build_date;

        binary_package_info() = default;
        binary_package_info(const binary_package_info&) = default;
        binary_package_info(binary_package_info&&) = default;
        binary_package_info(const package_base& base, std::time_t build_date)
            : package_base(base), build_date(build_date) {}

        ~binary_package_info() override = default;


        void print() const override;
        virtual std::string to_file() const;

        static std::optional<binary_package_info> parse_file(const std::string& filename);
    };

    struct binary_package {
        std::string path;
        binary_package_info pkg;

        bool install() const;

        static std::optional<binary_package> load(std::string path);
    };

    struct installed_package : binary_package_info {
        std::time_t install_date;

        installed_package() = default;
        installed_package(const installed_package&) = default;
        installed_package(installed_package&&) = default;
        installed_package(const binary_package_info& base, std::time_t install_date)
            : binary_package_info(base), install_date(install_date) {}

        ~installed_package() override = default;

        void print() const override;
        std::string to_file() const override;
        bool uninstall() const;
        std::list<std::string> get_files() const;

        static bool                             is_installed(std::string_view name);
        static std::optional<installed_package> parse_file(const std::string& filename);
        static std::optional<installed_package> parse_local(std::string_view name);
        static std::set<installed_package>      parse_local();
        static std::list<std::string>           get_files(std::string_view name);
        static std::vector<installed_package>   resolve(const std::vector<std::string>& args);
        static std::size_t                      estimate_size(std::string_view name);
        static std::size_t                      estimate_size(const std::vector<std::string>& names);
    };

    conflictsdb conflictsdb_read();
    void conflictsdb_write(const conflictsdb& db);

    // INLINE FUNCTIONS
    template<class T>
    inline std::string make_pkglist(const std::vector<T>& pkgs, std::string_view format_str = "{}") {
        std::string str{};
        for (const auto& pkg : pkgs) {
            str += ' ';
            str += fmt::format(format_str, pkg);
        }
        return str;
    }
    inline bool package_base::is_provided() const noexcept {
        return !provided_by.empty();
    }
    inline bool package_base::is_provider_of(std::string_view n) const noexcept {
        return n == name || std::find(begin(provides), end(provides), n) != end(provides);
    }
    inline bool package_base::conflicts_with(std::string_view n) const noexcept {
        return !is_provider_of(n) && std::find(begin(conflicts), end(conflicts), n) != end(conflicts);
    }
    inline bool package_base::operator<(const package_base& other) const noexcept {
        return name < other.name;
    }
    inline std::list<std::string> installed_package::get_files() const {
        return get_files(name);
    }
    inline std::size_t installed_package::estimate_size(const std::vector<std::string>& names) {
        return std::accumulate(begin(names),
                               end(names),
                               std::size_t{0},
                               [](const auto& sum, const auto& name) {
                                   return sum + estimate_size(name);
                               });
    }
}

template<>
struct fmt::formatter<minipkg2::package_base> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        auto it = ctx.begin();
        for (; it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
            case 'n':
            case 'v':
                mode = *it;
                break;
            default:
                throw format_error("bad format specifier");
            }
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const minipkg2::package_base& pkg, FormatContext& ctx) {
        switch (mode) {
        case 'n':
            return format_to(ctx.out(), "{}", pkg.name);
        case 'v':
            return format_to(ctx.out(), "{}:{}", pkg.name, pkg.version);
        default:
            throw format_error("unknown mode");
        }
    }
private:
    char mode = 'n';
};

template<>
struct fmt::formatter<minipkg2::source_package> : fmt::formatter<minipkg2::package_base> {};
template<>
struct fmt::formatter<minipkg2::binary_package_info> : fmt::formatter<minipkg2::package_base> {};
template<>
struct fmt::formatter<minipkg2::installed_package> : fmt::formatter<minipkg2::package_base> {};

#endif /* FILE_MINIPKG2_PACKAGE_HPP */
