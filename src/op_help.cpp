#include "cmdline.hpp"
#include "print.hpp"

namespace minipkg2::cmdline::operations {
    struct help_operation : operation {
        help_operation() : operation{"help", " [operation]", "Show some help."} {}
        int operator()(const std::vector<std::string>& args) override;
    };
    static help_operation op_help{};
    operation* help = &op_help;

    int help_operation::operator()(const std::vector<std::string>& args) {
        static constexpr unsigned help_len = 40;
        if (args.size() > 1) {
            fmt::print("Usage: minipkg2 help [operation]\n");
            return 1;
        }

        const auto print_opt = [](const option& opt) {
            switch (opt.type) {
            case option::ALIAS:
                fmt::print("  {:{}}Alias for {}.\n", opt.name, help_len, opt.value);
                break;
            case option::ARG:
                fmt::print("  {} <ARG>{:{}}{}\n", opt.name, "", help_len - 6 - opt.name.size(), opt.description);
                break;
            case option::BASIC:
                fmt::print("  {:{}}{}\n", opt.name, help_len, opt.description);
                break;
            }
        };

        if (args.size() == 0) {
            fmt::print("Micro-Linux Package Manager 2\n\nUsage:\n  minipkg2 [...] <operation> [...]\n\nOperations:\n");
            for (const auto* op : operation::operations) {
                fmt::print("  {}{}{:{}}{}\n", op->name, op->usage, "", help_len - op->name.length() - op->usage.length(), op->description);
            }
            fmt::print("\nGlobal options:\n");
            for (const auto& opt : option::global)
                print_opt(opt);
        } else {
            const auto& op2 = operation::get_op(args[0]);

            fmt::print("Micro-Linux Package Manager 2\n\nUsage:\n  minipkg2 {}{}\n\nDescription:\n  {}\n", op2.name, op2.usage, op2.description);

            if (op2.options.size() != 0) {
                fmt::print("\nOptions:\n");
                for (const auto& opt : op2.options)
                    print_opt(opt);
            }
        }
        fmt::print("\nWritten by Benjamin St??rz <benni@stuerz.xyz>\n");
        return 0;
    }
}
