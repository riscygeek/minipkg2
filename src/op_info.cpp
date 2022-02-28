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

        if (args.empty()) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        for (const auto& name : args) {
            std::unique_ptr<package_base> pkg;

            if (opt_local) {
                auto result = installed_package::parse_local(name);
                if (!result.has_value()) {
                    printerr(color::ERROR, "Invalid package: {}", name);
                    return 1;
                }
                pkg = std::make_unique<installed_package>(std::move(result.value()));
            } else if (opt_repo) {
                auto result = source_package::parse_repo(name);
                if (!result.has_value()) {
                    printerr(color::ERROR, "Invalid package: {}", name);
                    return 1;
                }
                pkg = std::make_unique<source_package>(std::move(result.value()));
            } else {
                auto result = installed_package::parse_local(name);
                if (result.has_value()) {
                    pkg = std::make_unique<installed_package>(std::move(result.value()));
                } else {
                    auto result2 = source_package::parse_repo(name);
                    if (!result2.has_value()) {
                        printerr(color::ERROR, "Invalid package: {}", name);
                        return 1;
                    }
                    pkg = std::make_unique<source_package>(std::move(result2.value()));
                }
            }

            pkg->print();
        }

        return 0;
    }
}
