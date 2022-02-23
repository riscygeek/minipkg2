#include "package.hpp"
#include "cmdline.hpp"
#include "print.hpp"

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
            std::cerr << star<color::ERROR> << "Either --repo or --local must be selected.\n";
            return 1;
        }

        if (opt_files && opt_repo) {
            std::cerr << star<color::ERROR> << "Options --files and --repo are incompatible.\n";
            return 1;
        }

        if (opt_upgradable && (opt_files || opt_repo)) {
            std::cerr << star<color::ERROR> << "Option --upgradable is incompatible with --repo and --files.";
            return 1;
        }

        if (opt_files && args.size() == 0) {
            std::cerr << star<color::ERROR> << "Option --files expects 1 or more arguments.\n";
            return 1;
        }

        std::vector<package> pkgs;

        if (opt_repo) {
            pkgs = package::parse_repo();
        } else {
            pkgs = package::parse_local();
        }

        if (opt_files) {
            std::cerr << star<color::ERROR> << "Unsupported option: --files.\n";
        } else if (opt_upgradable) {
            std::cerr << star<color::ERROR> << "Unsupported option: --upgradable.\n";
        } else {
            for (const auto& pkg : pkgs) {
                std::cout << pkg.name << ' ' << pkg.version << '\n';
            }
        }

        return 0;
    }
}
