#include <unistd.h>
#include <iostream>
#include "minipkg2.hpp"
#include "miniconf.hpp"
#include "cmdline.hpp"
#include "print.hpp"

namespace minipkg2::cmdline::operations {
    struct config_operation : operation {
        config_operation()
            : operation{
                "config",
                " --dump",
                "Manage the config file.",
                {
                    {option::BASIC, "--dump", "Dump the contents of the config file.", {}, false },
                }
            } {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static config_operation op_config;
    operation* config = &op_config;

    int config_operation::operator()(const std::vector<std::string>&) {
        const bool opt_dump = is_set("--dump");

        if (!opt_dump) {
            std::cerr << star<color::ERROR> << "Expected '--dump' option.\n";
            return 1;
        }


        minipkg2::miniconf::dump(std::cout, minipkg2::config);


        return 0;
    }
}
