#include <iostream>
#include "cmdline.hpp"

namespace minipkg2::cmdline::operations {
    struct help_operation : operation {
        help_operation() : operation{"help", " [operation]", "Show some help."} {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static help_operation op_help{};
    operation* help = &op_help;

    int help_operation::operator()(const std::vector<std::string>& args) {
        if (args.size() > 1) {
            std::cout << "Usage: minipkg2 help [operation]\n";
            return 1;
        }

        const auto print_opt = [](const option& opt) {
            const std::size_t len = opt.name.length();
            std::cout << "  " << opt.name;
            switch (opt.type) {
            case option::ALIAS:
                std::cout << std::string(30 - len, ' ') << "Alias for " << opt.value << ".\n";
                break;
            case option::ARG:
                std::cout << " <ARG>" << std::string(24 - len, ' ') << opt.description << '\n';
                break;
            case option::BASIC:
                std::cout << std::string(30 - len, ' ') << opt.description << '\n';
                break;
            }
        };

        if (args.size() == 0) {
            std::cout << "Micro-Linux Package Manager 2\n\nUsage:\n  minipkg2 [...] <operation> [...]\n\nOperations:\n";
            for (const auto* op : operation::operations) {
                std::cout << "  " << op->name << op->usage
                          << std::string(30 - op->name.length() - op->usage.length(), ' ')
                          << op->description << '\n';
            }
            std::cout << "\nGlobal options:\n";
            for (const auto& opt : option::global)
                print_opt(opt);
        } else {
            const auto& op2 = operation::get_op(args[0]);

            std::cout << "Micro-Linux Package Manager 2\n\nUsage:\n  minipkg2 "
                      << op2.name << op2.usage
                      << "\n\nDescription:\n  " << op2.description << '\n';

            if (op2.options.size() != 0) {
                std::cout << "\nOptions:\n";
                for (const auto& opt : op2.options)
                    print_opt(opt);
            }
        }
        std::cout << "\nWritten by Benjamin Stürz <benni@stuerz.xyz>\n";
        return 0;
    }
}
