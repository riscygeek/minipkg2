#include <stdexcept>
#include <cstring>
#include <cstdio>
#include "minipkg2.hpp"
#include "cmdline.hpp"
#include "print.hpp"

namespace minipkg2 {
    verbosity_level verbosity = verbosity_level::NORMAL;
}

namespace minipkg2::cmdline {
    using namespace std::literals;

    // Global variables
    std::vector<option> option::global = {
        { option::ARG,   "--root",      "Change the path to the root filesystem. (default: '/')",   {},     false },
        { option::ARG,   "--host",      "Set the target architecture for cross-compilation.",       {},     false },
        { option::BASIC, "-h",          "Print this page.",                                         {},     false },
        { option::ALIAS, "--help",      {},                                                         "-h",   false },
        { option::BASIC, "--version",   "Print information about the version of this program.",     {},     false },
        { option::BASIC, "-v",          "Verbose output.",                                          {},     false },
        { option::ALIAS, "--verbose",   {},                                                         "-v",   false },
        { option::BASIC, "-q",          "Suppress output.",                                         {},     false },
        { option::ALIAS, "--quiet",     {},                                                         "-q",   false },
        { option::ARG,   "-j",          "Specify the number of concurrent worker threads.",         {},     false },
        { option::ALIAS, "--jobs",      {},                                                         "-j",   false },
    };
    std::vector<operation*> operation::operations = {
        operations::clean,
        operations::config,
        operations::download,
        operations::help,
        operations::install,
        operations::list,
        operations::purge,
        operations::remove,
        operations::repo,
        operations::show,
    };

    // Utility Functions

    option& operation::get_option(std::string_view name) {
        for (option& opt : options) {
            if (opt.name == name) {
                return opt;
            }
        }
        throw std::invalid_argument("Option not found: '"s + std::string{name} + '\'');
    }
    bool operation::is_set(std::string_view option) {
        return get_option(option).selected;
    }
    bool option::global_is_set(std::string_view name) {
        option& opt = option::resolve(nullptr, get_global(name));
        return opt.selected;
    }
    option& option::get_global(std::string_view name) {
        for (option& opt : global) {
            if (opt.name == name)
                return opt;
        }
        throw std::invalid_argument("Invalid option: " + std::string{name});
    }
    operation& operation::get_op(std::string_view name) {
        for (auto* op : operations) {
            if (op->name == name)
                return *op;
        }
        throw std::invalid_argument("Invalid operation: " + std::string{name});
    }
    option& option::resolve(operation* op, option& opt) {
        if (opt.type != ALIAS)
            return opt;


        if (op != nullptr) {
            return resolve(op, op->get_option(opt.value));
        } else {
            return resolve(nullptr, get_global(opt.value));
        }
    }

    // Actual Pparsing Function

    int parse(int argc, char** argv) {
        std::vector<std::string> args{};
        operation* op = nullptr;
        bool stop_opts = false;
        int idx;

        const auto print_usage = [] {
            std::fputs("Usage: minipkg2 --help\n", stderr);
        };
        const auto parse_opt1 = [&](option& opt, option& real_opt) {
            const std::string_view arg = argv[idx];
            if (real_opt.type == option::ARG) {
                if (opt.name == arg.substr(0, opt.name.length())) {
                    real_opt.selected = true;
                    if (arg[opt.name.length()] == '=') {
                        real_opt.value = arg.substr(opt.name.length() + 1);
                    } else {
                        const auto next = argv[idx + 1];
                        if (!next || next[0] == '-')
                            throw parse_error("Option '" + std::string{opt.name} + "' expects an argument.");
                        real_opt.value = next;
                        ++idx;
                    }
                    return true;
                }
                return false;
            } else {
                if (arg == opt.name) {
                    real_opt.selected = true;
                    return true;
                }
                return false;
            }
        };
        const auto parse_opt = [&] {
            if (stop_opts || argv[idx][0] != '-' || argv[idx][1] == '\0') {
                args.emplace_back(argv[idx]);
                return;
            }
            if (!std::strcmp(argv[idx], "--")) {
                stop_opts = true;
                return;
            }
            bool found_opt = false;
            if (op) {
                for (option& opt : op->options) {
                    option& real_opt = option::resolve(op, opt);
                    if (parse_opt1(opt, real_opt)) {
                        found_opt = true;
                        break;
                    }
                }
            }

            if (!found_opt) {
                for (option& opt : option::global) {
                    option& real_opt = option::resolve(nullptr, opt);
                    if (parse_opt1(opt, real_opt)) {
                        found_opt = true;
                        break;
                    }
                }
            }

            if (!found_opt)
                throw parse_error("Unrecognized option: "s + argv[idx]);
        };

        for (idx = 1; idx < argc && argv[idx][0] == '-'; ++idx)
            parse_opt();

        if (idx != argc) {
            op = &operation::get_op(argv[idx]);

            for (++idx; idx < argc; ++idx) {
                parse_opt();
            }
        }

        // Handle global options.
        if (option::global_is_set("-h")) {
            if (op)
                args.emplace(args.begin(), op->name);
            return operation::get_op("help")(args);
        }

        if (option::global_is_set("--version")) {
            print_version();
            return 0;
        }

        if (option::global_is_set("-q")) {
            verbosity = verbosity_level::QUIET;
        } else if (option::global_is_set("-v")) {
            verbosity = verbosity_level::VERBOSE;
        }

        if (auto& opt = option::get_global("--root"); opt.selected) {
            set_root(opt.value);
        }

        if (auto& opt = option::get_global("--host"); opt.selected) {
            host = opt.value;
        }

        if (auto& opt = option::get_global("-j"); opt.selected) {
            std::size_t endp = std::string::npos;
            int jobs;

            try {
                jobs = std::stoi(opt.value, &endp);
            } catch (...) {
                goto jobs_failed;
            }

            if (endp < opt.value.size()) {
            jobs_failed:;
                throw parse_error("Failed to parse --jobs="s + opt.value);
            }

            minipkg2::jobs = static_cast<std::size_t>(jobs);
        }

        if (!op) {
            print_usage();
            return 1;
        }

        return (*op)(args);
    }
}
