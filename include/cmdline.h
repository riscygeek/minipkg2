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
   int(*run)(const struct operation*);
};


extern const struct operation operations[];
extern const size_t num_operations;

int parse_cmdline(int argc, char* argv[]);

int op_install(const struct operation*);
int op_remove(const struct operation*);
int op_purge(const struct operation*);
int op_list(const struct operation*);
int op_info(const struct operation*);
int op_download_source(const struct operation*);
int op_clean_cache(const struct operation*);

#endif /* FILE_MINIPKG2_CMDLINE_H */
