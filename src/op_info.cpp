#include "cmdline.hpp"
#include "package.hpp"
#include "print.hpp"

namespace minipkg2::cmdline::operations {
    struct info_operation : operation {
        info_operation()
            : operation{
                "info",
                " [options] <package>",
                "Show information about a package.",
                {
                    {option::BASIC, "--repo",       "Show installable packages.",           {}, false },
                    {option::BASIC, "--local",      "Show installed packages.",             {}, false },
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static info_operation op_info;
    operation* info = &op_info;
    int info_operation::operator()(const std::vector<std::string>& args) {
        const bool opt_local = is_set("--local");
        const bool opt_repo  = is_set("--repo");

        if (opt_repo && opt_local) {
            printerr(color::ERROR, "Either --repo or --local must be selected.");
            return 1;
        }

        if (args.size() == 0) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        std::vector<package> pkgs;
        for (const auto& name : args) {
            package pkg;
            try {
                if (opt_local) {
                    pkg = package::parse(package::source::LOCAL, name);
                } else if (opt_repo) {
                    pkg = package::parse(package::source::REPO, name);
                } else {
                    try {
                        pkg = package::parse(package::source::LOCAL, name);
                    } catch (...) {
                        pkg = package::parse(package::source::REPO, name);
                    }
                }
            } catch (const parse_error& e) {
                printerr(color::ERROR, "Failed to find package {}: {}", name, e.what());
                return 1;
            } catch (const std::exception& e) {
                printerr(color::ERROR, "Failed to parse package {}: {}", name, e.what());
                return 2;
            }

            const auto print_line = [](std::string_view name, std::size_t n, const std::string* strs) {
                fmt::print("{:30}: ", name);
                for (std::size_t i = 0; i < n; ++i)
                    fmt::print("{}", strs[i]);
                fmt::print("\n");
            };
            print_line("Name",                  1,                      &pkg.name);
            print_line("Version",               1,                      &pkg.version);
            print_line("URL",                   1,                      &pkg.url);
            print_line("Description",           1,                      &pkg.description);
            print_line("Sources",               pkg.sources.size(),     pkg.sources.data());
            print_line("Build Dependencies",    pkg.bdepends.size(),    pkg.bdepends.data());
            print_line("Runtime Dependencies",  pkg.rdepends.size(),    pkg.rdepends.data());
            print_line("Provides",              pkg.provides.size(),    pkg.provides.data());
            print_line("Conflicts",             pkg.conflicts.size(),   pkg.conflicts.data());
            print_line("Features",              pkg.features.size(),    pkg.features.data());
        }

        return 0;
    }
}
