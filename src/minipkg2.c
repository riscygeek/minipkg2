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
   init_log();
}

void set_repodir(const char* dir) {
   free(repodir);
   repodir = xstrdup(dir);
}

void init_self(void) {
   self = xreadlink("/proc/self/exe");
}

#ifndef HAS_LIBCURL
#define HAS_LIBCURL false
#endif

void print_version(void) {
   puts("Micro-Linux Package Manager\n");
   puts("Version: " VERSION);
   puts("\nBuild:");
   puts("  system:     " BUILD_SYS);
   puts("  prefix:     " CONFIG_PREFIX);
   puts("  libdir:     " CONFIG_PREFIX "/" CONFIG_LIBDIR);
   puts("  sysconfdir: " CONFIG_PREFIX "/" CONFIG_SYSCONFDIR);
   puts("  date:       " __DATE__);
   puts("  time:       " __TIME__);
   puts("\nFeatures:");
   printf("  libcurl:    %s\n", HAS_LIBCURL ? "true" : "false");
   puts("\nWritten by Benjamin Stürz <benni@stuerz.xyz>.");
}
