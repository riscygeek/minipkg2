#include <stdlib.h>
#include "minipkg2.h"
#include "cmdline.h"
#include "print.h"
#include "utils.h"
#include "git.h"

struct cmdline_option repo_options[] = {
   { "--init",    OPT_ARG,    "Initialize the repository.",      {NULL},  },
   { "--sync",    OPT_BASIC,  "Synchronize the repository.",     {NULL},  },
   { "--branch",  OPT_ARG,    "Change to a different branch..",  {NULL},  },
   { NULL },
};

static void check_git(void) {
   char* version = git_version();
   if (version == NULL)
      fail("Runtime dependency 'git' is not installed.");
   free(version);
}

static int no_repo(void) {
   error("Repo is not initialized.");
   return 1;
}

defop(repo) {
   (void)args;
   (void)num_args;
   check_git();
   if (op_is_set(op, "--init")) {
      rm_rf(repodir);
      if (!git_clone(op_get_arg(op, "--init"), repodir, op_get_arg(op, "--branch")))
         fail("Failed to initialize repo.");
      return 0;
   }

   if (op_is_set(op, "--sync")) {
      if (!is_dir(repodir))
         return no_repo();
      if (!git_pull(repodir))
         fail("Failed to sync repo.");
      return 0;
   }

   if (op_is_set(op, "--branch")) {
      if (!is_dir(repodir))
         return no_repo();
      const char* branch = op_get_arg(op, "--branch");
      char* cur_branch = git_branch(repodir);
      if (strcmp(branch, cur_branch) != 0) {
         if (!git_checkout(repodir, branch))
            fail("Failed to change to branch '%s'", branch);
      } else {
         warn("Already on branch '%s'", branch);
      }
      free(cur_branch);
      return 0;
   }

   if (is_dir(repodir)) {
      char* branch = git_branch(repodir);
      info("Repo is on branch '%s'", branch);
      free(branch);
   } else {
      return no_repo();
   }

   return 0;
}
