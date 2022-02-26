#ifndef FILE_MINIPKG2_PACKAGE_HPP
#define FILE_MINIPKG2_PACKAGE_HPP
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


        static package parse(const std::string& filename);
        static package parse(source from, std::string_view name);
        static std::vector<package> parse_local();
        static std::vector<package> parse_repo();
    };
}

#endif /* FILE_MINIPKG2_PACKAGE_HPP */