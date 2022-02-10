#include <unistd.h>
#include "utils.hpp"
#include "git.hpp"

namespace minipkg2::git {
    std::string version() {
        auto [ec, reply] = xpread("git --version 2>/dev/null");

        if (ec != 0)
            return {};

        const std::string prefix = "git version ";
        return starts_with(reply, prefix) ? reply.substr(prefix.length()) : std::string{};
    }

    static std::string verbosity_option() {
        // TODO: waiting for verbosity support.
        return "";
    }

    static int xsystem(std::string&& s) {
        return std::system(s.c_str());
    }

    bool clone(const std::string& url, const std::string& dest, const std::string& branch) {
        std::string cmd = "git clone " + verbosity_option() + " '" + std::string{url} + "' '" + dest + '\'';
        if (branch.size() != 0)
            cmd += " --branch '" + branch + '\'';

        return std::system(cmd.c_str()) == 0;
    }
    bool pull(const std::string& repo) {
        return xsystem("cd '" + repo + "' && git pull " + verbosity_option()) == 0;
    }
    bool checkout(const std::string& repo, const std::string& branch) {
        return xsystem("cd '" + repo + "' && git checkout " + verbosity_option() + " '" + branch + '\'') == 0;
    }
    std::string branch(const std::string& repo) {
        auto [ec, reply] = xpread("cd '" + repo + "' && git branch --show-current");
        return ec == 0 ? reply : std::string{};
    }
    bool sync(const std::string& url, const std::string& dest) {
        return access(dest.c_str(), F_OK) == 0 ? pull(dest) : clone(url, dest, {});
    }
}
