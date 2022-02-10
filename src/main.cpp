#include "cmdline.hpp"

int main(int argc, char* argv[]) {
    try {
        return minipkg2::cmdline::parse(argc, argv);
    } catch (const minipkg2::cmdline::parse_error& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 100;
    }
}
