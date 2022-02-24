#include <unistd.h>
#include "minipkg2.hpp"
#include "cmdline.hpp"
#include "utils.hpp"
#include "print.hpp"
#include "git.hpp"

namespace minipkg2::cmdline::operations {
    struct repo_operation : operation {
        repo_operation()
            : operation{
                "repo",
                " [options]",
                "Manage the repository.",
                {
                    {option::ARG,   "--init",   "Initialize the repository.",       {}, false },
                    {option::BASIC, "--sync",   "Synchronize the repository.",      {}, false },
                    {option::ARG,   "--branch", "Change to a different branch.",    {}, false },
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static repo_operation op_repo;
    operation* repo = &op_repo;

    int repo_operation::operator()(const std::vector<std::string>&) {
        std::string version = git::version();
        if (version.empty()) {
            printerr(color::ERROR, "Runtime dependency 'git' is not installed.");
            return 1;
        }

        if (const auto& opt = get_option("--init")) {
            rm_rf(repodir);
            if (!git::clone(opt.value, repodir, get_option("--branch").value)) {
                printerr(color::ERROR, "Failed to initialize repo.");
                return 1;
            }
            return 0;
        }

        if (::access(repodir.c_str(), F_OK) != 0) {
            printerr(color::WARN, "Repo is not initialized!");
            return 1;
        }

        if (is_set("--sync")) {
            if (!git::pull(repodir)) {
                printerr(color::ERROR, "Failed to sync repo.");
                return 1;
            }
            return 0;
        }

        if (const auto& opt = get_option("--branch")) {
            auto branch = git::branch(repodir);
            if (branch != opt.value) {
                if (!git::checkout(repodir, opt.value)) {
                    printerr(color::ERROR, "Failed to change to branch '{}'.", opt.value);
                    return 1;
                }
            } else {
                printerr(color::WARN, "Already on branch '{}'.", branch);
            }
            return 0;
        }

        printerr(color::INFO, "Repo is on branch '{}'.", git::branch(repodir));
        return 0;
    }
}
