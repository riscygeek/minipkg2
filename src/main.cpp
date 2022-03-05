#include <cassert>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include "minipkg2.hpp"
#include "miniconf.hpp"
#include "cmdline.hpp"
#include "print.hpp"

#include "bashconfig.hpp"

int main(int argc, char* argv[]) {
    if (!minipkg2::init_self())
        return 1;

    try {
        std::FILE* file = std::fopen("test.bash", "r");
        assert(file != nullptr);
        const auto conf = minipkg2::bashconfig::read(file);
        std::fclose(file);
        minipkg2::bashconfig::write(stdout, conf);
    } catch (const std::exception& e) {
        fmt::print("{}\n", e.what());
    }
    return 0;

    try {
        return minipkg2::cmdline::parse(argc, argv);
    } catch (const minipkg2::cmdline::parse_error& e) {
        fmt::print(stderr, "{}\n", e.what());
        return 1;
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 100;
    }
}
