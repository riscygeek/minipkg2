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

        if (args.empty()) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        printerr(color::LOG, "Resolving packages...");
        const auto skip_policy = opt_skip ? resolve_skip_policy::ALWAYS : resolve_skip_policy::DEPEND;
        const auto pkgs = source_package::resolve(args, !opt_no_deps, skip_policy);

        if (!opt_force) {
            bool success = true;
            printerr(color::LOG, "Checking for conflicts...");
            for (const auto& pkg : pkgs) {
                success &= pkg.check_conflicts();
            }
            if (!success)
                return 1;
        }

        if (pkgs.empty()) {
            printerr(color::LOG, "Nothing done.");
            return 0;
        }

        printerr(color::LOG, "");
        printerr(color::LOG, "Packages ({}){}", pkgs.size(), make_pkglist(pkgs));
        printerr(color::LOG, "");

        if (!opt_yes) {
            if (!yesno("Proceed with download?", true))
                return 1;
            printerr(color::LOG, "");
        }

        if (opt_clean) {
            printerr(color::LOG, "Cleaning up...");
            for (const auto& pkg : pkgs) {
                rm_rf(fmt::format("{}/{}-{}", builddir, pkg.name, pkg.version));
            }
        }

        printerr(color::LOG, "Downloading sources...");
        if (!source_package::download(pkgs))
            return 1;

        printerr(color::LOG, "");
        printerr(color::LOG, "Processing packages..");

        for (std::size_t i = 0; i < pkgs.size(); ++i) {
            const auto& pkg = pkgs[i];
            printerr(color::LOG, "({}/{}) Building {:v}...", i+1, pkgs.size(), pkg);
            const auto path_binpkg = fmt::format("{0}/{1}-{2}/{1}:{2}.bmpkg.tar.gz", builddir, pkg.name, pkg.version);
            const auto filesdir = fmt::format("{}/{}/files", pkgdir, pkg.name);

            const auto result = pkg.build(path_binpkg, filesdir);
            if (!result.has_value()) {
                return 1;
            }
            const auto binpkg = result.value();
            printerr(color::LOG, "({}/{}) Installing {:v}...", i+1, pkgs.size(), binpkg.pkg);
            binpkg.install();
        }


        return 0;
    }
}
