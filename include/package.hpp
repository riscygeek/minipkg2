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

        static package parse(const std::string& filename);
        static package parse(source from, std::string_view name);
        static std::vector<package> parse_local();
        static std::vector<package> parse_repo();
    };
}

template<>
struct fmt::formatter<minipkg2::package> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template<typename FormatContext>
    auto format(const minipkg2::package& pkg, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}:{}", pkg.name, pkg.version);
    }
};

#endif /* FILE_MINIPKG2_PACKAGE_HPP */
