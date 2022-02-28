#include "minipkg2.hpp"
#include "cmdline.hpp"
#include "utils.hpp"

namespace minipkg2::cmdline::operations {
    struct clean_operation : operation {
        clean_operation()
            : operation{
                "clean",
                "",
                "Remove build files.",
                {}
            } {}
        int operator()(const std::vector<std::string>&) override {
            rm_rf(builddir);
            return 0;
        }
    };
    static clean_operation op_clean;
    operation* clean = &op_clean;
}
