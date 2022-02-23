#include <unistd.h>
#include <iostream>
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
            std::cerr << star<color::ERROR> << "Runtime dependency 'git' is not installed.\n";
            return 1;
        }

        if (const auto& opt = get_option("--init")) {
            rm_rf(repodir);
            if (!git::clone(opt.value, repodir, get_option("--branch").value)) {
                std::cerr << star<color::ERROR> << "Failed to initialize repo.\n";
                return 1;
            }
            return 0;
        }

        if (::access(repodir.c_str(), F_OK) != 0) {
            std::cerr << star<color::WARN> << "Repo is not initialized!\n";
            return 1;
        }

        if (is_set("--sync")) {
            if (!git::pull(repodir)) {
                std::cerr << star<color::ERROR> << "Failed to sync repo.\n";
                return 1;
            }
            return 0;
        }

        if (const auto& opt = get_option("--branch")) {
            auto branch = git::branch(repodir);
            if (branch != opt.value) {
                if (!git::checkout(repodir, opt.value)) {
                    std::cerr << star<color::ERROR> << "Failed to change to branch '" << opt.value << "'\n";
                    return 1;
                }
            } else {
                std::cerr << star<color::WARN> << "Already on branch '" << branch << "'\n";
            }
            return 0;
        }

        std::cout << star<color::INFO> << "Repo is on branch '" << git::branch(repodir) << "'\n";

        return 0;
    }
}
