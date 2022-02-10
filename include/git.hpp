#ifndef FILE_MINIPKG2_GIT_HPP
#define FILE_MINIPKG2_GIT_HPP
#include <string>

namespace minipkg2::git {
    // Get the version of git.
    std::string version();

    // Clone a git repository.
    bool clone(const std::string& url, const std::string& dest, const std::string& branch);

    // Synchronize a local git repository.
    bool pull(const std::string& repo);

    // Switch to a different branch.
    bool checkout(const std::string& repo, const std::string& branch);

    // Get the current branch.
    std::string branch(const std::string& repo);

    // If the repo already exists, git_pull(dest), otherwise git_clone(url, dest, NULL);
    bool sync(const std::string& url, const std::string& dest);
}

#endif /* FILE_MINIPKG2_GIT_HPP */
