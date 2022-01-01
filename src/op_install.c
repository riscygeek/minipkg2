#include "minipkg2.h"
#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

struct cmdline_option install_options[] = {
   { NULL },
};

struct package* lel;
static void add_package(struct package*** pkgs, struct package* pkg) {
   for (size_t i = 0; i < buf_len(*pkgs); ++i) {
      if ((*pkgs)[i] == pkg)
         return;
   }
   buf_push(*pkgs, pkg);
}

static void find_dependencies(struct package*** pkgs, struct package_info** infos, const struct package* pkg) {
   for (size_t i = 0; i < buf_len(pkg->depends); ++i) {
      struct package_info* dep = find_package_info(infos, pkg->depends[i]);
      if (!dep || !dep->pkg)
         fail("Dependency %s not found.", pkg->depends[i]);
      find_dependencies(pkgs, infos, dep->pkg);
      add_package(pkgs, dep->pkg);
   }
}

defop(install) {
   (void)op;
   struct package_info* infos = NULL;
   find_packages(&infos, PKG_REPO);

   struct package** pkgs = NULL;
   for (size_t i = 0; i < num_args; ++i) {
      struct package_info* info = find_package_info(&infos, args[i]);
      if (!info || !info->pkg)
         fail("Package %s not found", args[i]);
      find_dependencies(&pkgs, &infos, info->pkg);
      add_package(&pkgs, info->pkg);
   }

   for (size_t i = 0; i < buf_len(pkgs); ++i) {
      puts(pkgs[i]->name);
   }
   return 0;
}
