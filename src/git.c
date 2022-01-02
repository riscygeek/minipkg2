#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include "minipkg2.h"
#include "print.h"
#include "utils.h"
#include "git.h"

char* git_version(void) {
   char* reply = xpread("git --version");

   static const char prefix[] = "git version ";
   if (!starts_with(reply, prefix))
      return free(reply), NULL;

   char* str = xstrdup(reply + sizeof(prefix) - 1);
   free(reply);

   return str;
}
static const char* verbosity_option(void) {
   if (verbosity <= 1) {
      return "--quiet";
   } else if (verbosity == 2) {
      return "";
   } else {
      return "--verbose";
   }
}
bool git_clone(const char* url, const char* dest, const char* branch) {
   char* cmd = xstrcatl("git clone ", verbosity_option(), " '", url, "' '", dest, "'");
   if (branch) {
      char* old = cmd;
      cmd = xstrcatl(cmd, " --branch '", branch, "'");
      free(old);
   }
   const int ec = system(cmd);
   free(cmd);
   return ec == 0;
}

bool git_pull(const char* repo) {
   char* cmd = xstrcatl("cd '", repo, "' && git pull ", verbosity_option());
   const int ec = system(cmd);
   free(cmd);
   return ec == 0;
}

bool git_checkout(const char* repo, const char* branch) {
   char* cmd = xstrcatl("cd '", repo, "' && git checkout ", verbosity_option(), " '", branch, "'");
   const int ec = system(cmd);
   free(cmd);
   return ec;
}
char* git_branch(void) {
   return xpread("git branch --show-current");
}
