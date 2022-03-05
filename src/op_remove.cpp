#include "cmdline.hpp"
#include "package.hpp"
#include "quickdb.hpp"
#include "print.hpp"
#include "utils.hpp"

namespace minipkg2::cmdline::operations {
    struct remove_operation : operation {
        remove_operation()
            : operation{
                "remove",
                " [options] <package(s)>",
                "Remove packages.",
                {
                    {option::BASIC, "-y",           "Don't ask for confirmation.",          {}, false},
                    {option::ALIAS, "--yes",        {},                                     "-y", false},
                    {option::BASIC, "--purge",      "Purge the package.",                   {}, false},
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static remove_operation op_remove;
    operation* remove = &op_remove;

    int remove_operation::operator()(const std::vector<std::string>& args) {
        const bool opt_yes      = is_set("-y");
        const bool opt_purge    = is_set("--purge");

        if (args.empty()) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        printerr(color::LOG, "Resolving packages...");
        auto pkgs = installed_package::resolve(args);

        if (!opt_purge) {
            printerr(color::LOG, "Checking for reverse-dependencies...");
            auto db = quickdb::read("rdeps");
            bool success = true;
            for (const auto& pkg : pkgs) {
                const auto check = [&db, &success, &args](const std::string& name) {
                    auto it = db.find(name);
                    if (it != db.end() && !it->second.empty()) {
                        for (const auto& x : it->second) {
                            if (!contains(args, x)) {
                                printerr(color::ERROR, "Package '{}' depends on '{}'.", x, name);
                                success = false;
                            }
                        }
                    }
                };
                check(pkg.name);
                for (const auto& p : pkg.provides)
                    check(p);
            }
            if (!success)
                return 1;
        }

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
