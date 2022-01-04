#include "minipkg2.h"
#include "utils.h"

char* root        = NULL; // /
char* pkgdir      = NULL; // $root/var/db/minipkg2/packages
char* repodir     = NULL; // $root/var/db/minipkg2/repo
char* builddir    = NULL; // $root/var/tmp/minipkg2

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
