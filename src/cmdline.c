#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minipkg2.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

static struct cmdline_option no_options[] = {
   { NULL },
};


extern struct cmdline_option info_options[];
extern struct cmdline_option list_options[];

const struct operation operations[] = {
   { "help",            no_options,       &op_help,   false, 0, },
   { "install",         no_options,       &op_unimp,  true,  1, },
   { "remove",          no_options,       &op_unimp,  true,  1, },
   { "purge",           no_options,       &op_unimp,  true,  1, },
   { "list",            list_options,     &op_list,   true,  0, },
   { "info",            info_options,     &op_info,   true,  1, },
   { "download-source", no_options,       &op_unimp,  true,  1, },
   { "clean-cache",     no_options,       &op_unimp,  false, 0, },
};

const size_t num_operations = arraylen(operations);

static void print_help(void) {
   puts("Micro-Linux Package Manager 2\n");
   puts("Usage: minipkg2 <operation> [...]\n");
   puts("Operations:");
   puts("  minipkg2 help");
   puts("  minipkg2 install <package(s)>");
   puts("  minipkg2 remove <package(s)>");
   puts("  minipkg2 purge <package(s)>");
   puts("  minipkg2 list [list-options]");
   puts("  minipkg2 info [info-options] <package>");
   puts("  minipkg2 download-source <package(s)>");
   puts("  minipkg2 clean-cache");
   puts("\nWritten by Benjamin Stürz <benni@stuerz.xyz>");
}

static void print_usage(void) {
   fputs("Usage: minipkg2 --help\n", stderr);
}

defop(help) {
   (void)op;
   (void)args;
   (void)num_args;
   print_help();
   return 0;
}
defop(unimp) {
   (void)op;
   (void)args;
   (void)num_args;
   fail("This operation is currently not supported");
   return 1;
}

int parse_cmdline(int argc, char* argv[]) {
   bool stop_opts = false;
   int arg;
   for (arg = 1; arg < argc && argv[arg][0] == '-'; ++arg) {
      if (!strcmp(argv[arg], "-")) {
         fail("Arguments must be placed after the operation.");
      } else if (!strcmp(argv[arg], "--")) {
         stop_opts = true;
         ++arg;
         break;
      } else if (!strcmp(argv[arg], "--help")) {
         print_help();
         return 0;
      } else if (!strcmp(argv[arg], "--version")) {
         puts(VERSION);
         return 0;
      } else if (starts_with(argv[arg], "--root=")) {
         set_root(argv[arg] + 7);
         if (!*root)
            fail("Expected argument for --root=");
      } else {
         fail("Unrecognized command-line option '%s'", argv[arg]);
      }
   }

   if (arg == argc) {
      print_usage();
      return 1;
   }

   const struct operation* op = NULL;
   for (size_t i = 0; i < num_operations; ++i) {
      if (!strcmp(argv[arg], operations[i].name)) {
         op = &operations[i];
         break;
      }
   }
   if (!op) {
      fail("Unrecognized operation mode '%s'", argv[arg]);
   }

   char** args = NULL;
   for (++arg; arg < argc; ++arg) {
      if (!stop_opts && argv[arg][0] == '-' && argv[arg][1] != '\0') {
         bool found_opt = false;
         if (!strcmp(argv[arg], "--")) {
            stop_opts = true;
            continue;
         }
         for (size_t i = 0; op->options[i].option; ++i) {
            struct cmdline_option* opt = &op->options[i];
            if (opt->has_arg) {
               const size_t len = strlen(opt->option);
               if (!strncmp(argv[arg], opt->option, len)) {
                  found_opt = true;
                  opt->arg = argv[arg] + len;
                  break;
               }
            } else {
               if (!strcmp(argv[arg], opt->option)) {
                  found_opt = true;
                  opt->selected = true;
                  break;
               }
            }
         }
         if (!found_opt)
            fail("Unrecognized sub-option '%s'", argv[arg]);
      } else {
         if (!op->supports_args)
            fail("Operation %s does not support arguments.", op->name);
         buf_push(args, argv[arg]);
      }
   }

   if (buf_len(args) < op->min_args)
      fail("Operation %s needs at least %zu argument(s).", op->name, op->min_args);

   return op->run(op, args, buf_len(args));
}

const struct cmdline_option* op_get_opt(const struct operation* op, const char* name) {
   for (size_t i = 0; op->options[i].option; ++i) {
      const struct cmdline_option* opt = &op->options[i];
      if (!strcmp(opt->option, name))
         return opt;
   }
   return NULL;
}
bool op_is_set(const struct operation* op, const char* name) {
   const struct cmdline_option* opt = op_get_opt(op, name);
   if (!opt)
      fail("op_is_set(): No such option: %s", name);
   return opt->has_arg ? !!opt->arg : opt->selected;
}
