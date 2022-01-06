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
extern struct cmdline_option clean_options[];
extern struct cmdline_option download_options[];
extern struct cmdline_option build_options[];

struct cmdline_option global_options[] = {
   { "--root",    OPT_ARG,    "Set the path of the root filesystem. (default: '/')",   {NULL}, },
   { "-h",        OPT_BASIC,  "Print this page.",                                      {NULL}, },
   { "--help",    OPT_ALIAS,  NULL,                                                    {"-h"}, },
   { "--version", OPT_BASIC,  "Print information about the version.",                  {NULL}, },
   { "-v",        OPT_BASIC,  "Verbose output.",                                       {NULL}, },
   { "-vv",       OPT_BASIC,  "Extra verbose output.",                                 {NULL}, },
   { "--verbose", OPT_ALIAS,  NULL,                                                    {"-v"}, },
   { "-q",        OPT_BASIC,  "Suppress output.",                                      {NULL}, },
   { "--quiet",   OPT_ALIAS,  NULL,                                                    {"-q"}, },
   {NULL},
};

const struct operation operations[] = {
   { "help",            no_options,       &op_help,   true,  0, " [operation]",        "Show some help.", },
   { "install",         install_options,  &op_install,true,  1, " <package(s)>",       "Build and install packages.", },
   { "remove",          remove_options,   &op_remove, true,  1, " <package(s)>",       "Remove packages.", },
   { "purge",           purge_options,    &op_purge,  true,  1, " <package(s)>",       "Remove packages without checking for reverse-dependencies.", },
   { "list",            list_options,     &op_list,   true,  0, " [options]",          "List packages or files of packages.", },
   { "info",            info_options,     &op_info,   true,  1, " [options] <package>","Show information about a package.", },
   { "download",        download_options, &op_download,true, 1, " <package(s)>",       "Download the source files of a package.", },
   { "clean",           no_options,       &op_clean,  false, 0, "",                    "Remove build files.", },
   { "repo",            repo_options,     &op_repo,   false, 0, " [options]",          "Manage the repository." },
   { "build",           build_options,    &op_build,  true,  1, " <package(s)>",       "Build packages." },
};

const size_t num_operations = arraylen(operations);

static void print_usage(void) {
   fputs("Usage: minipkg2 --help\n", stderr);
}

static struct cmdline_option* resolve_opt(const struct operation*, struct cmdline_option*);
static void parse_opt(const struct operation*, int*, char*[], char***, bool*);


int parse_cmdline(int argc, char* argv[]) {
   bool stop_opts = false;
   int arg;
   for (arg = 1; arg < argc && argv[arg][0] == '-'; ++arg) {
      parse_opt(NULL, &arg, argv, NULL, &stop_opts);
   }
   
   const struct operation* op = NULL;
   char** args = NULL;
   if (arg != argc) {
      op = get_op(argv[arg]);
      if (!op) {
         fail("Unrecognized operation mode '%s'", argv[arg]);
      }

      for (++arg; arg < argc; ++arg) {
         parse_opt(op, &arg, argv, &args, &stop_opts);
      }

      if (buf_len(args) < op->min_args)
         fail("Operation %s needs at least %zu argument(s).", op->name, op->min_args);
   }

   // Handle global options.
   if (op_is_set(NULL, "-h")) {
      op_help(NULL, NULL, 0);
      return 0;
   }
   if (op_is_set(NULL, "--version")) {
#ifdef HAS_LIBCURL
         const bool has_libcurl = HAS_LIBCURL;
#else
         const bool has_libcurl = false;
#endif
         puts(VERSION);
         printf("Has libcurl: %s\n", has_libcurl ? "true" : "false");
         return 0;
   }
   if (op_is_set(NULL, "-q")) {
      verbosity = V_QUIET;
   } else if (op_is_set(NULL, "-vv")) {
      verbosity = V_EXTRA_VERBOSE;
   } else if (op_is_set(NULL, "-v")) {
      verbosity = V_VERBOSE;
   }
   if (op_is_set(NULL, "--root")) {
      set_root(op_get_arg(NULL, "--root"));
   }

   if (!op) {
      print_usage();
      return 1;
   }

   return op->run(op, args, buf_len(args));
}

struct cmdline_option* op_get_opt(const struct operation* op, const char* name) {
   struct cmdline_option* options = op != NULL ? op->options : global_options;
   for (size_t i = 0; options[i].option; ++i) {
      struct cmdline_option* opt = &options[i];
      if (!strcmp(opt->option, name)) {
         if (opt->type == OPT_ALIAS)
            return op_get_opt(op, opt->alias);
         return opt;
      }
   }
   return op ? op_get_opt(NULL, name) : NULL;
}
bool op_is_set(const struct operation* op, const char* name) {
   const struct cmdline_option* opt = op_get_opt(op, name);
   if (!opt)
      fail("op_is_set(): No such option: %s", name);
   return opt->type == OPT_ARG ? !!opt->arg : opt->selected;
}
static struct cmdline_option* resolve_opt(const struct operation* op, struct cmdline_option* opt) {
   while (opt->type == OPT_ALIAS) {
      struct cmdline_option* new_opt = op_get_opt(op, opt->alias);
      if (!opt)
         fail("Option '%s' is alias of non-existent option '%s'", opt->option, opt->alias);
      opt = new_opt;
   }
   return opt;
}


const struct operation* get_op(const char* name) {
   for (size_t i = 0; i < num_operations; ++i) {
      if (!strcmp(name, operations[i].name))
         return &operations[i];
   }
   return NULL;
}

static bool parse_opt1(int* arg, char* argv[], struct cmdline_option* opt, struct cmdline_option* real_opt) {
   if (real_opt->type == OPT_ARG) {
      const size_t len = strlen(opt->option);
      if (!strncmp(argv[*arg], opt->option, len)) {
         if (argv[*arg][len] == '=') {
            real_opt->arg = argv[*arg] + len + 1;
         } else {
            const char* next = argv[*arg + 1];
            if (!next || next[0] == '-')
               fail("Option '%s' expects an argument.", argv[*arg]);
            real_opt->arg = next;
            ++*arg;
         }
         return true;
      }
   } else {
      if (!strcmp(argv[*arg], opt->option)) {
         real_opt->selected = true;
         return true;
      }
   }
   return false;
}
static void parse_opt(const struct operation* op, int* arg, char* argv[], char*** args, bool* stop_opts) {
   if (!*stop_opts && argv[*arg][0] == '-' && argv[*arg][1] != '\0') {
      bool found_opt = false;
      if (!strcmp(argv[*arg], "--")) {
         *stop_opts = true;
         return;
      }
      if (op) {
         for (size_t i = 0; op->options[i].option; ++i) {
            struct cmdline_option* opt = &op->options[i];
            struct cmdline_option* real_opt = resolve_opt(op, opt);
            if (parse_opt1(arg, argv, opt, real_opt)) {
               found_opt = true;
               break;
            }
         }
      }
      if (!found_opt) {
         for (size_t i = 0; global_options[i].option; ++i) {
            struct cmdline_option* opt = &global_options[i];
            struct cmdline_option* real_opt = resolve_opt(op, opt);
            if (parse_opt1(arg, argv, opt, real_opt)) {
               found_opt = true;
               break;
            }
         }
      }
      if (!found_opt)
         fail("Unrecognized option '%s'", argv[*arg]);
   } else {
      if (!op->supports_args)
         fail("Operation %s does not support arguments.", op->name);
      buf_push(*args, argv[*arg]);
   }
}
