#include <stdlib.h>
#include <stdio.h>
#include "cmdline.h"
#include "print.h"
#include "utils.h"

static void print_opt(const struct cmdline_option* opt) {
   const int len = (int)strlen(opt->option);
   if (opt->type == OPT_ALIAS) {
      printf("  %s%*sAlias for %s.\n", opt->option, 30 - len, "", opt->alias);
   } else if (opt->type == OPT_ARG) {
      printf("  %s <ARG>%*s%s\n", opt->option, 24 - len, "", opt->description);
   } else {
      printf("  %s%*s%s\n", opt->option, 30 - len, "", opt->description);
   }
}


defop(help) {
   (void)op;
   if (num_args > 1) {
      puts("Usage: minipkg2 help [operation]");
      return 1;
   }
   puts("Micro-Linux Package Manager 2");
   if (num_args == 0) {
      puts("\nUsage:\n  minipkg2 [global-options] <operation> [...]");
      puts("\nOperations:");
      for (size_t i = 0; i < num_operations; ++i) {
         const struct operation* op = &operations[i];
         printf("  %s%s\n", op->name, op->usage);
      }
      puts("\nGlobal options:");
      for (size_t i = 0; global_options[i].option; ++i) {
         print_opt(&global_options[i]);
      }
   } else if (num_args == 1) {
      const struct operation* op2 = get_op(args[0]);
      if (!op2) {
         error("Invalid operation: %s", args[0]);
         return 1;
      }
      puts("\nUsage:");
      printf("  minipkg2 %s%s\n", op2->name, op2->usage);
      printf("\nDescription:\n  %s\n", op2->description);

      if (op2->options[0].option != NULL) {
         puts("\nOptions:");
         for (size_t i = 0; op2->options[i].option != NULL; ++i) {
            print_opt(&op2->options[i]);
         }
      }
   }
   puts("\nWritten by Benjamin Stürz <benni@stuerz.xyz>");
   return 0;
}
