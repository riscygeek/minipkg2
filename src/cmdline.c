#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline.h"
#include "config.h"
#include "print.h"

static struct cmdline_option no_options[] = {
   { NULL },
};

static struct cmdline_option list_options[] = {
   { "--installed", false,    },
   { "--available", false,    },
   { "--files=",    true,     },
   { NULL },
};

static struct cmdline_option info_options[] = {
   { "--local",     false,    },
   { "--repo",      false,    },
   { NULL },
};

static int op_help(const struct operation*);
static int op_unimp(const struct operation*);

const struct operation operations[] = {
   { "help",            no_options,       &op_help,   },
   { "install",         no_options,       &op_unimp,  },
   { "remove",          no_options,       &op_unimp,  },
   { "purge",           no_options,       &op_unimp,  },
   { "list",            list_options,     &op_unimp,  },
   { "info",            info_options,     &op_unimp,  },
   { "download-source", no_options,       &op_unimp,  },
   { "clean-cache",     no_options,       &op_unimp,  },
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

static int op_help(const struct operation* op) {
   (void)op;
   print_help();
   return 0;
}
static int op_unimp(const struct operation* op) {
   (void)op;
   fail("This operation is currently not supported");
   return 1;
}

int parse_cmdline(int argc, char* argv[]) {
   int arg;
   for (arg = 1; arg < argc && argv[arg][0] == '-'; ++arg) {
      if (!strcmp(argv[arg], "--help")) {
         print_help();
         return 0;
      } else if (!strcmp(argv[arg], "--version")) {
         puts(VERSION);
         return 0;
      } else if (!strncmp(argv[arg], "--root=", 7)) {
         root = argv[arg] + 7;
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

   for (++arg; arg < argc && argv[arg][0] == '-'; ++arg) {
      bool found_opt = false;
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
      if (!found_opt) {
         fail("Unrecognized sub-option '%s'", argv[arg]);
      }
   }
   return op->run(op);
}
