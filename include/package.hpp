#ifndef FILE_MINIPKG2_PACKAGE_HPP
#define FILE_MINIPKG2_PACKAGE_HPP
#include <fmt/format.h>
#include <string_view>
#include <string>
#include <vector>

namespace minipkg2 {
    struct package {
        enum class source {
            OTHER,
            LOCAL,
            REPO,
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

        bool is_installed() const;
        bool download() const;
        bool is_provided() const noexcept { return !provided_by.empty(); }
        bool operator<(const package& other) const noexcept { return name < other.name; }

        static package parse(const std::string& filename);
        static package parse(source from, std::string_view name);
        static std::vector<package> parse_local();
        static std::vector<package> parse_repo();
        static std::vector<package> resolve(const std::vector<std::string>& names, bool resolve_deps = false, bool skip_installed = false);
        static bool is_installed(std::string_view name);
        static std::string make_pkglist(const std::vector<package>& pkgs, bool include_version = false);
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


#endif /* FILE_MINIPKG2_PACKAGE_HPP */
