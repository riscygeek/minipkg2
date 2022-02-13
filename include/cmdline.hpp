#ifndef FILE_MINIPKG2_CMDLINE_HPP
#define FILE_MINIPKG2_CMDLINE_HPP
#include <initializer_list>
#include <string_view>
#include <stdexcept>
#include <optional>
#include <string>
#include <vector>

namespace minipkg2::cmdline {
    struct parse_error : std::invalid_argument {
        parse_error(std::string what) : std::invalid_argument(what) {}
        parse_error(const parse_error&) = default;
        parse_error(parse_error&&) = default;
        ~parse_error() = default;
    };


    // Global Variables

    struct operation;
    struct option {
        enum type {
            BASIC,  // eg. -y
            ARG,    // eg. --branch=BRANCH
            ALIAS,  // eg. --yes
        };

        const type type;
        const std::string_view name;
        const std::string_view description;
        std::string value;
        bool selected;

        operator bool() const noexcept { return selected; }

        static std::vector<option> global;
        static bool global_is_set(std::string_view name);
        static option& get_global(std::string_view name);
        static option& resolve(operation* op, option& opt);
    };


    struct operation {
        const std::string_view name;
        const std::string_view usage;
        const std::string_view description;
        std::vector<option> options;

        operation(std::string_view name, std::string_view usage, std::string_view description, const std::initializer_list<option>& opts = {})
            : name{name}, usage{usage}, description{description}, options{opts} {}

        virtual ~operation() = default;

        virtual int operator()(const std::vector<std::string>& args) = 0;

        option& get_option(std::string_view name);
        bool is_set(std::string_view option);

        static std::vector<operation*> operations;
        static operation& get_op(std::string_view name);
    };

    // XXX: Please also look into cmdline.cpp when adding new operations.
    namespace operations {
        extern operation* help;
        extern operation* repo;
    }

    int parse(int argc, char* argv[]);
}

#endif /* FILE_MINIPKG2_CMDLINE_HPP */
