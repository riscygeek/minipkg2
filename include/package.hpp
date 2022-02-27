#ifndef FILE_MINIPKG2_PACKAGE_HPP
#define FILE_MINIPKG2_PACKAGE_HPP
#include <fmt/format.h>
#include <string_view>
#include <optional>
#include <string>
#include <vector>
#include <set>

namespace minipkg2 {
    struct bin_package;
    struct package {
        enum class source {
            OTHER,
            LOCAL,
            REPO,
        };
        enum class skip_policy {
            NEVER,
            TOP,
            DEEP,
        };

        source from;
        std::string filename;
        std::string name;
        std::string version;
        std::string url;
        std::string description;
        std::string provided_by;
        std::vector<std::string> sources;
        std::vector<std::string> bdepends;
        std::vector<std::string> rdepends;
        std::vector<std::string> provides;
        std::vector<std::string> conflicts;
        std::vector<std::string> features;

        inline static std::set<package> local_packages{};
        inline static std::set<package> repo_packages{};

        bool is_installed() const;
        bool download() const;
        bool is_provided() const noexcept { return !provided_by.empty(); }
        bool operator<(const package& other) const noexcept { return name < other.name; }
        bool is_provided(std::string_view n) const;
        bool conflicts_with(std::string_view n) const;
        bool check_conflicts() const;
        std::optional<bin_package> build(std::string_view binpkg, const std::string& filesdir) const;

        static package parse(const std::string& filename);
        static package parse(source from, std::string_view name);
        static const std::set<package>& parse_local();
        static const std::set<package>& parse_repo();
        static std::vector<package> resolve(const std::vector<std::string>& names, bool resolve_deps = false, skip_policy skip = skip_policy::NEVER);
        static bool is_installed(std::string_view name);
        static std::string make_pkglist(const std::vector<package>& pkgs, bool include_version = false);
        static bool download(const std::vector<package>& pkgs);
    };
    struct package_info {
        std::string name;
        std::string version;
        std::string url;
        std::string description;
        std::vector<std::string> rdepends;
        std::vector<std::string> provides;
        std::vector<std::string> conflicts;

        bool write_to(const std::string& filename) const;

        static package_info from(const package& pkg);
        static package_info parse_file(const std::string& filename);
        static package_info parse_local(const std::string& name);
    };
    struct bin_package {
        std::string filename;
        package_info pkg;
    };

}

template<>
struct fmt::formatter<minipkg2::package> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        auto it = ctx.begin();
        for (; it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
            case 'v':
                print_version = true;
                break;
            default:
                throw format_error("bad format specifier");
            }
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const minipkg2::package& pkg, FormatContext& ctx) {
        if (print_version) {
            return format_to(ctx.out(), "{}:{}", pkg.name, pkg.version);
        } else {
            return format_to(ctx.out(), "{}", pkg.name);
        }
    }
private:
    bool print_version = false;
};

template<>
struct fmt::formatter<minipkg2::package_info> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        auto it = ctx.begin();
        for (; it != ctx.end() && *it != '}'; ++it) {
            switch (*it) {
            case 'v':
                print_version = true;
                break;
            default:
                throw format_error("bad format specifier");
            }
        }
        return it;
    }
    template<typename FormatContext>
    auto format(const minipkg2::package_info& pkg, FormatContext& ctx) {
        if (print_version) {
            return format_to(ctx.out(), "{}:{}", pkg.name, pkg.version);
        } else {
            return format_to(ctx.out(), "{}", pkg.name);
        }
    }
private:
    bool print_version = false;
};


#endif /* FILE_MINIPKG2_PACKAGE_HPP */
