#include <iostream>
#include "minipkg2.hpp"
#include "cmdline.hpp"

int main(int argc, char* argv[]) {

    minipkg2::set_root("/");

    try {
        return minipkg2::cmdline::parse(argc, argv);
    } catch (const minipkg2::cmdline::parse_error& e) {
        std::cerr << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 100;
    }
}
