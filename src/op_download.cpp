#include "minipkg2.hpp"
#include "cmdline.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "print.hpp"

namespace minipkg2::cmdline::operations {
    struct download_operation : operation {
        download_operation()
            : operation{
                "download",
                " [options] <package(s)>",
                "Download package sources.",
                {
                    {option::BASIC, "-y",           "Don't ask for confirmation.",          {},     false },
                    {option::ALIAS, "--yes",        {},                                     "-y",   false },
                    {option::BASIC, "--deps",       "Also download the dependencis.",       {},     false },
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static download_operation op_download;
    operation* download = &op_download;

    int download_operation::operator()(const std::vector<std::string>& args) {
        const bool opt_yes  = is_set("-y");
        const bool opt_deps = is_set("--deps");

        if (args.size() == 0) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        const auto pkgs = resolve(args, opt_deps, resolve_skip_policy::NEVER);

        printerr(color::LOG, "");
        printerr(color::LOG, "Packages ({}){}", pkgs.size(), make_pkglist(pkgs));
        printerr(color::LOG, "");

        if (!opt_yes) {
            if (!yesno("Proceed with download?", true))
                return 1;
            printerr(color::LOG, "");
        }

        printerr(color::LOG, "Downloading sources...");

        return !source_package::download(pkgs);
    }
}
