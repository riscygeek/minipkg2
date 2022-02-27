#include "minipkg2.hpp"
#include "cmdline.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "print.hpp"

namespace minipkg2::cmdline::operations {
    struct install_operation : operation {
        install_operation()
            : operation{
                "install",
                " [options] <package>",
                "Install packages.",
                {
                    { option::BASIC, "-y",              "Don't ask for confirmation.",      {},     false },
                    { option::ALIAS, "--yes",           {},                                 "-y",   false },
                    { option::BASIC, "--clean",         "Perform a clean build.",           {},     false },
                    { option::BASIC, "--no-deps",       "Don't check for dependencies.",    {},     false },
                    { option::BASIC, "-s",              "Skip installed packages.",         {},     false },
                    { option::ALIAS, "--skip-installed",{},                                 "-s",   false },
                    { option::BASIC, "--force",         "Don't check for conflicts.",       {},     false },
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static install_operation op_install{};
    operation* install = &op_install;

    int install_operation::operator()(const std::vector<std::string>& args) {
        const bool opt_yes      = is_set("-y");
        const bool opt_clean    = is_set("--clean");
        const bool opt_no_deps  = is_set("--no-deps");
        const bool opt_skip     = is_set("-s");
        const bool opt_force    = is_set("--force");

        printerr(color::LOG, "Resolving packages...");
        const auto skip_policy = opt_skip ? package::skip_policy::TOP : package::skip_policy::NEVER;
        const auto pkgs = package::resolve(args, !opt_no_deps, skip_policy);

        if (!opt_force) {
            printerr(color::LOG, "Checking for conflicts...");
            for (const auto& pkg : pkgs) {
                pkg.check_conflicts();
            }
        }

        printerr(color::LOG, "");
        printerr(color::LOG, "Packages ({}){}", pkgs.size(), package::make_pkglist(pkgs));
        printerr(color::LOG, "");

        if (!opt_yes) {
            if (!yesno("Proceed with download?", true))
                return 1;
            printerr(color::LOG, "");
        }

        printerr(color::LOG, "Downloading sources...");
        if (!package::download(pkgs))
            return 1;

        printerr(color::LOG, "");
        printerr(color::LOG, "Processing packages..");

        for (std::size_t i = 0; i < pkgs.size(); ++i) {
            const auto& pkg = pkgs[i];
            printerr(color::LOG, "({}/{}) Building {}...", i+1, pkgs.size(), pkg);
            const auto path_binpkg = fmt::format("{0}/{1}-{2}/{1}:{2}.bmpkg.tar.gz", builddir, pkg.name, pkg.version);
            const auto filesdir = fmt::format("{}/{}/files", pkgdir, pkg.name);

            const auto result = pkg.build(path_binpkg, filesdir);
            if (!result.has_value()) {
                return 1;
            }
            const auto binpkg = result.value();
            printerr(color::LOG, "({}/{}) Installing {}...", i+1, pkgs.size(), binpkg.pkg);
        }


        return 0;
    }
}
