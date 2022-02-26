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

        if (opt_deps) {
            printerr(color::ERROR, "Option --deps is currently not supported.");
            return 1;
        }

        if (args.size() == 0) {
            printerr(color::ERROR, "At least 1 argument expected.");
            return 1;
        }

        std::vector<package> pkgs{};
        std::string pkg_list{};

        for (const auto& name : args) {
            auto pkg = package::parse(package::source::REPO, name);
            pkg_list += ' ';
            pkg_list += pkg.name;
            pkgs.push_back(std::move(pkg));
        }

        printerr(color::LOG, "");
        printerr(color::LOG, "Packages ({}){}", pkgs.size(), pkg_list);
        printerr(color::LOG, "");

        if (!opt_yes) {
            if (!yesno("Proceed with download?", true))
                return 1;
            printerr(color::LOG, "");
        }

        printerr(color::LOG, "Downloading sources...");

        bool success = true;
        for (std::size_t i = 0; i < pkgs.size(); ++i) {
            const auto& pkg = pkgs[i];
            printerr(color::LOG, "({}/{}) Downloading {}", i+1, pkgs.size(), pkg);

            success &= pkg.download();
        }

        return !success;
    }
}
