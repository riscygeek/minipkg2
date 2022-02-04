#include <unistd.h>
#include <stdlib.h>
#include "minipkg2.h"
#include "utils.h"
#include "print.h"

char* root        = NULL; // /
char* pkgdir      = NULL; // $root/var/db/minipkg2/packages
char* repodir     = NULL; // $root/var/db/minipkg2/repo
char* builddir    = NULL; // $root/var/tmp/minipkg2
const char* host  = NULL; // eg. x86_64-micro-linux-gnu
const char* jobs  = NULL; // How many concurrent per-package compile jobs
char* self        = NULL; // Path to the minipkg2 executable

void set_root(const char* dir) {
   free(root);
   free(pkgdir);
   free(builddir);
   free(repodir);
   root        = xstrdup(dir);
   pkgdir      = xstrcat(root, "/var/db/minipkg2/packages");
   repodir     = xstrcat(root, "/var/db/minipkg2/repo");
   builddir    = xstrcat(root, "/var/tmp/minipkg2");
}

void set_repodir(const char* dir) {
   free(repodir);
   repodir = xstrdup(dir);
}

void init_self(void) {
   if (!self) {
      self = xmalloc(BUFSIZ);
      const ssize_t rv = readlink("/proc/self/exe", self, BUFSIZ);
      if (rv < 0)
         fail_errno("Failed to readlink('/proc/self/exe')");

      if (rv >= BUFSIZ)
         fail("Failed to readlink('/proc/self/exe'): Buffer too small");
   }
}
