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
extern struct cmdline_option install_options[];
extern struct cmdline_option purge_options[];
extern struct cmdline_option remove_options[];
extern struct cmdline_option repo_options[];

const struct operation operations[] = {
   { "help",            no_options,       &op_help,   true,  0, " [operation]",        "Show some help.", },
   { "install",         install_options,  &op_install,true,  1, " <package(s)>",       "Build and install packages.", },
   { "remove",          remove_options,   &op_remove, true,  1, " <package(s)>",       "Remove packages.", },
   { "purge",           purge_options,    &op_purge,  true,  1, " <package(s)>",       "Remove packages without checking for reverse-dependencies.", },
   { "list",            list_options,     &op_list,   true,  0, " [options]",          "List packages or files of packages.", },
   { "info",            info_options,     &op_info,   true,  1, " [options] <package>","Show information about a package.", },
   { "download-source", no_options,       &op_unimp,  true,  1, " <package(s)>",       "Download the source files of a package.", },
   { "clean-cache",     no_options,       &op_unimp,  false, 0, "",                    "Remove build files.", },
   { "repo",            repo_options,     &op_repo,   false, 0, "[options]",           "Manage the repository." },
};

const size_t num_operations = arraylen(operations);

static void print_opt(const char* opt, const char* descr) {
   const size_t len = strlen(opt);
   printf("  %s%*s%s\n", opt, 30 - (int)len, "", descr);
}

static void print_help(void) {
   puts("Micro-Linux Package Manager 2");
   puts("\nUsage:\n  minipkg2 [global-options] <operation> [...]");
   puts("\nOperations:");
   for (size_t i = 0; i < num_operations; ++i) {
      const struct operation* op = &operations[i];
      printf("  %s%s\n", op->name, op->usage);
   }
   puts("\nGlobal options:");
   print_opt("--root=ROOT", "Set the path to the root. (default: '/')");
   print_opt("-h,--help", "Print this page.");
   print_opt("--version", "Print the version.");
   print_opt("-v,--verbose", "Verbose output.");
   print_opt("-vv", "More verbose output.");
   print_opt("-q,--quiet", "Suppress output.");
   puts("Note: global options must precede the operation.");
   puts("\nWritten by Benjamin Stürz <benni@stuerz.xyz>");
}

static const struct operation* get_op(const char* name);
static void print_usage(void) {
   fputs("Usage: minipkg2 --help\n", stderr);
}

static struct cmdline_option* resolve_opt(const struct operation* op, const struct cmdline_option* opt) {
   while (opt->type == OPT_ALIAS) {
      struct cmdline_option* new_opt = op_get_opt(op, opt->alias);
      if (!opt)
         fail("Option '%s' is alias of non-existent option '%s'", opt->option, opt->alias);
      opt = new_opt;
   }
   return (struct cmdline_option*)opt;
}

defop(help) {
   (void)op;
   if (num_args == 0) {
      print_help();
   } else if (num_args == 1) {
      const struct operation* op2 = get_op(args[0]);
      if (!op2) {
         error("Invalid operation: %s", args[0]);
         return 1;
      }
      puts("Micro-Linux Package Manager 2");
      puts("\nUsage:");
      printf("  minipkg2 %s%s\n", op2->name, op2->usage);
      printf("\nDescription:\n  %s\n", op2->description);

      if (op2->options[0].option != NULL) {
         puts("\nOptions:");
         for (size_t i = 0; op2->options[i].option != NULL; ++i) {
            const struct cmdline_option* opt = &op2->options[i];
            if (opt->type == OPT_ALIAS) {
               char* msg = xstrcatl("Alias for ", opt->alias, ".");
               print_opt(opt->option, msg);
               free(msg);
            } else if (opt->type == OPT_ARG) {
               char* option = xstrcat(opt->option, " <ARG>");
               print_opt(option, opt->description);
               free(option);
            } else {
               print_opt(opt->option, opt->description);
            }
         }
      }
      puts("\nWritten by Benjamin Stürz <benni@stuerz.xyz>");
   } else {
      puts("Usage: minipkg2 help [operation]");
   }
   return 0;
}
defop(unimp) {
   (void)op;
   (void)args;
   (void)num_args;
   fail("This operation is currently not supported");
   return 1;
}

static const struct operation* get_op(const char* name) {
   for (size_t i = 0; i < num_operations; ++i) {
      if (!strcmp(name, operations[i].name))
         return &operations[i];
   }
   return NULL;
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
      } else if (xstreql(argv[arg], "-h", "--help")) {
         print_help();
         return 0;
      } else if (!strcmp(argv[arg], "--version")) {
#ifdef HAS_LIBCURL
         const bool has_libcurl = HAS_LIBCURL;
#else
         const bool has_libcurl = false;
#endif
         puts(VERSION);
         printf("Has libcurl: %s\n", has_libcurl ? "true" : "false");
         return 0;
      } else if (starts_with(argv[arg], "--root=")) {
         set_root(argv[arg] + 7);
         if (!*root)
            fail("Expected argument for --root=");
      } else if (!strcmp(argv[arg], "--verbose")) {
         verbosity = 2;
      } else if (xstreql(argv[arg], "-q", "--quiet")) {
         verbosity = 0;
      } else if (starts_with(argv[arg], "-v")) {
         for (size_t i = 1; argv[arg][i] == 'v'; ++i)
            ++verbosity;
      } else {
         fail("Unrecognized command-line option '%s'", argv[arg]);
      }
   }

   if (arg == argc) {
      print_usage();
      return 1;
   }

   const struct operation* op = get_op(argv[arg]);
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
            struct cmdline_option* real_opt = resolve_opt(op, opt);
            if (real_opt->type == OPT_ARG) {
               const size_t len = strlen(opt->option);
               if (!strncmp(argv[arg], opt->option, len)) {
                  found_opt = true;
                  if (argv[arg][len] == '=') {
                     real_opt->arg = argv[arg] + len + 1;
                  } else {
                     const char* next = argv[arg + 1];
                     if (!next || next[0] == '-')
                        fail("Option '%s' expects an argument.", argv[arg]);
                     real_opt->arg = next;
                     ++arg;
                  }
                  break;
               }
            } else {
               if (!strcmp(argv[arg], opt->option)) {
                  found_opt = true;
                  real_opt->selected = true;
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

struct cmdline_option* op_get_opt(const struct operation* op, const char* name) {
   for (size_t i = 0; op->options[i].option; ++i) {
      struct cmdline_option* opt = &op->options[i];
      if (!strcmp(opt->option, name)) {
         if (opt->type == OPT_ALIAS)
            return op_get_opt(op, opt->alias);
         return opt;
      }
   }
   return NULL;
}
bool op_is_set(const struct operation* op, const char* name) {
   const struct cmdline_option* opt = op_get_opt(op, name);
   if (!opt)
      fail("op_is_set(): No such option: %s", name);
   return opt->type == OPT_ARG ? !!opt->arg : opt->selected;
}
