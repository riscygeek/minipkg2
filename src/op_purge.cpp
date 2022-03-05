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
        remove->get_option("-y").selected = is_set("-y");
        remove->get_option("--purge").selected = true;
        return (*remove)(args);
    }
}
