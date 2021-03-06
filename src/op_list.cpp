#include <unistd.h>
#include "minipkg2.hpp"
#include "package.hpp"
#include "cmdline.hpp"
#include "print.hpp"
#include "utils.hpp"

namespace minipkg2::cmdline::operations {
    struct list_operation : operation {
        list_operation()
            : operation{
                "list",
                " [options]",
                "List packages or their files.",
                {
                    {option::BASIC, "--repo",       "List installable packages.",           {}, false },
                    {option::BASIC, "--local",      "List installed packages.",             {}, false },
                    {option::BASIC, "--files",      "List files of installed packages.",    {}, false },
                    {option::BASIC, "--upgradable", "List upgradable packages.",            {}, false },
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static list_operation op_list;
    operation* list = &op_list;

    int list_operation::operator()(const std::vector<std::string>& args) {
        const bool opt_repo         = is_set("--repo");
        const bool opt_local        = is_set("--local");
        const bool opt_files        = is_set("--files");
        const bool opt_upgradable   = is_set("--upgradable");

        if (opt_repo && opt_local) {
            printerr(color::ERROR, "Either --repo or --local must be selected.");
            return 1;
        }

        if (opt_files && opt_repo) {
            printerr(color::ERROR, "Options --files and --repo are incompatible.");
            return 1;
        }

        if (opt_upgradable && (opt_files || opt_repo)) {
            printerr(color::ERROR, "Options --upgradable is incompatible with --repo and --files.");
            return 1;
        }

        if (opt_files && args.size() == 0) {
            printerr(color::ERROR, "Option --files expects 1 or more arguments.");
            return 1;
        }

        if (opt_repo) {
            const auto& pkgs = source_package::parse_repo();
            for (const auto& pkg : pkgs) {
                fmt::print("{} {}\n", pkg.name, pkg.version);
            }
        } else {
            if (opt_files) {
                for (const auto& name : args) {
                    const auto path = fmt::format("{}/{}/files", pkgdir, name);
                    if (::access(path.c_str(), R_OK) != 0) {
                        printerr(color::ERROR, "Invalid package: {}.", name);
                        return 1;
                    }
                    cat(stdout, path);
                }
                return 0;
            }

            const auto& pkgs = installed_package::parse_local();

            if (opt_upgradable) {
                printerr(color::ERROR, "Unsupported option: --upgradable");
            } else {
                for (const auto& pkg : pkgs) {
                    fmt::print("{} {}\n", pkg.name, pkg.version);
                }
            }
        }
        return 0;
    }
}
