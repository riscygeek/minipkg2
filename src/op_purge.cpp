#include "cmdline.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "print.hpp"

namespace minipkg2::cmdline::operations {
    struct purge_operation : operation {
        purge_operation()
            : operation{
                "purge",
                " [-y] <package(s)>",
                "Remove packages without checking for reserve-dependencies.",
                {
                    {option::BASIC, "-y",           "Don't ask for confirmation.",          {}, false},
                    {option::ALIAS, "--yes",        {},                                     "-y", false},
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static purge_operation op_purge;
    operation* purge = &op_purge;

    int purge_operation::operator()(const std::vector<std::string>& args) {
        const bool opt_yes = is_set("-y");

        if (args.empty()) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        printerr(color::LOG, "Resolving packages...");
        auto pkgs = installed_package::resolve(args);
        printerr(color::LOG, "Estimating size...");
        const auto size = installed_package::estimate_size(args);

        printerr(color::LOG, "");
        printerr(color::LOG, "Packages ({}){}", pkgs.size(), make_pkglist(pkgs));
        printerr(color::LOG, "");
        printerr(color::LOG, "Estimated size: {}", fmt_size(size));

        if (!opt_yes) {
            if (!yesno("Proceed with removal?", true))
                return 1;
            printerr(color::LOG, "");
        }
        printerr(color::LOG, "Processing packages..");

        for (std::size_t i = 0; i < pkgs.size(); ++i) {
            const auto& pkg = pkgs[i];
            printerr(color::LOG, "({}/{}) Purging {:v}...", i+1, pkgs.size(), pkg);
            if (!pkg.uninstall()) {
                printerr(color::ERROR, "Failed to purge {}.", pkg);
                return 1;
            }
        }

        return 0;
    }
}
