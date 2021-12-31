#ifndef FILE_MINIPKG2_CMDLINE_H
#define FILE_MINIPKG2_CMDLINE_H
#include <stdbool.h>
#include <stddef.h>

struct cmdline_option {
   const char* option;
   bool has_arg;
   union {
      const char* arg;
      bool selected;
   };
};

struct operation {
   const char* name;
   struct cmdline_option* options;
   int(*run)(const struct operation*, char**, size_t);
   bool supports_args;
   size_t min_args;
};


extern const struct operation operations[];
extern const size_t num_operations;

int parse_cmdline(int argc, char* argv[]);

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

#endif /* FILE_MINIPKG2_CMDLINE_H */
