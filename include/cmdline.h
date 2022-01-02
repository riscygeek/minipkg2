#ifndef FILE_MINIPKG2_CMDLINE_H
#define FILE_MINIPKG2_CMDLINE_H
#include <stdbool.h>
#include <stddef.h>

enum cmdline_option_type {
   OPT_BASIC,     // eg. -y
   OPT_ARG,       // eg. --branch=BRANCH
   OPT_ALIAS,     // eg. --yes
};

struct cmdline_option {
   const char* option;
   enum cmdline_option_type type;
   const char* description;
   union {
      const char* arg;
      const char* alias;
      bool selected;
   };
};

struct operation {
   const char* name;
   struct cmdline_option* options;
   int(*run)(const struct operation*, char**, size_t);
   bool supports_args;
   size_t min_args;
   const char* usage;
   const char* description;
};


extern const struct operation operations[];
extern const size_t num_operations;

int parse_cmdline(int argc, char* argv[]);
struct cmdline_option* op_get_opt(const struct operation*, const char*);
bool op_is_set(const struct operation* op, const char* opt);

#define op_get_arg(op, name)  (op_get_opt(op, name)->arg)

#define defop(name) int op_##name(const struct operation* op, char** args, size_t num_args)

defop(help);
defop(unimp);
defop(install);
defop(remove);
defop(purge);
defop(list);
defop(info);
defop(download_source);
defop(clean_cache);
defop(repo);

#endif /* FILE_MINIPKG2_CMDLINE_H */
