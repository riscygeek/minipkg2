#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

struct cmdline_option download_options[] = {
   { "-y",              OPT_BASIC, "Don't ask for confirmation.",    {NULL}, },
   { "--yes",           OPT_ALIAS, NULL,                             {"-y"}, },
   {NULL},
};

defop(download) {
   (void)op;
   struct package_info* infos = NULL;
   find_packages(&infos, PKG_REPO);

   struct package** pkgs = NULL;
   for (size_t i = 0; i < num_args; ++i) {
      struct package_info* info = find_package_info(&infos, args[i]);
      if (!info || !info->pkg)
         fail("Package %s not found", args[i]);
      buf_push(pkgs, info->pkg);
   }

   const size_t num_pkgs = buf_len(pkgs);
   if (verbosity >= V_NORMAL) {
      log("");
      print(COLOR_LOG, "Packages (%zu)", num_pkgs);
      for (size_t i = 0; i < num_pkgs; ++i) {
         print(0, " %s", pkgs[i]->name);
      }
      println(0, "");
      log("");
   }

   if (!op_is_set(op, "-y")) {
      if (!yesno("Proceed with installation?", true))
         return 1;
      log("");
   }

   log("Downloading sources...");

   bool success = true;
   for (size_t i = 0; i < num_pkgs; ++i) {
      log("(%zu/%zu) Downloading %s:%s...",
            i+1, num_pkgs,
            pkgs[i]->name, pkgs[i]->version);
      if (!pkg_download_sources(pkgs[i]))
         success = false;
   }

   free_package_infos(&infos);
   return success ? 0 : 1;
}
