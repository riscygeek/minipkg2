#include <stdio.h>
#include "cmdline.h"
#include "package.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

struct cmdline_option purge_options[] = {
   {NULL},
};

static void add_package(struct package_info*** infos, struct package_info* info) {
   for (size_t i = 0; i < buf_len(*infos); ++i) {
      if ((*infos)[i] == info)
         return;
   }
   buf_push(*infos, info);
}

defop(purge) {
   (void)op;
   // Find all installed packages.
   struct package_info* infos = NULL;
   find_packages(&infos, PKG_LOCAL);

   // Find all packags to be purged.
   struct package_info** purgelist = NULL;
   bool failed = false;
   for (size_t i = 0; i < num_args; ++i) {
      struct package_info* info = find_package_info(&infos, args[i]);
      if (!info) {
         error("Package not found: %s", args[i]);
         failed = true;
      }
      add_package(&purgelist, info);
      // TODO: handle provided packages
   }
   if (failed)
      return 1;
   failed = false;

   log("Estimating size...");
   log("");
   size_t total_size = 0;
   for (size_t i = 0; i < buf_len(purgelist); ++i) {
      size_t sz;
      if (pkg_estimate_size(purgelist[i]->provided_name, &sz)) {
         total_size += sz;
      } else {
         warn("Failed to estimate size for %s", purgelist[i]->provided_name);
      }
   }

   print(COLOR_LOG, "Packages (%zu)", buf_len(purgelist));
   for (size_t i = 0; i < buf_len(purgelist); ++i) {
      fprintf(stderr, " %s", purgelist[i]->provider_name);
   }
   fputc('\n', stderr);
   log("");
   const char* unit;
   format_size(&total_size, &unit);
   log("Total Removal Size: %zu%s", total_size, unit);
   log("");
   if (!yesno("Do you want to purge these packages?", true))
      return 1;

   for (size_t i = 0; i < buf_len(purgelist); ++i) {
      const struct package_info* pi = purgelist[i];
      const struct package* pkg = pi->pkg;
      log("(%zu/%zu) Purging %s:%s...",
            i+1, buf_len(purgelist),
            pkg->name, pkg->version);
      if (!purge_package(pkg->name)) {
         error("Failed to purge %s.", pkg->name);
         return 1;
      }
   }

   return 0;
}
