#include <stdio.h>
#include "cmdline.h"
#include "package.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

struct cmdline_option purge_options[] = {
   { "-y",        OPT_BASIC, "Don't ask for confirmation.",    {NULL}, },
   { "--yes",     OPT_ALIAS, NULL,                             {"-y"}, },
   {NULL},
};
struct cmdline_option remove_options[] = {
   { "-y",        OPT_BASIC, "Don't ask for confirmation.",  {NULL}, },
   { "--yes",     OPT_ALIAS, NULL,                             {"-y"}, },
   { "--purge",   OPT_BASIC, "Purge the package.",           {NULL}, },
   {NULL},
};

static void add_package(struct package_info*** infos, struct package_info* info) {
   for (size_t i = 0; i < buf_len(*infos); ++i) {
      if ((*infos)[i] == info)
         return;
   }
   buf_push(*infos, info);
}
static int perform(const struct operation*, char**, size_t, bool);
defop(remove) {
   return perform(op, args, num_args, op_is_set(op, "--purge"));
}

defop(purge) {
   return perform(op, args, num_args, true);
}

static int perform(const struct operation* op, char** args, size_t num_args, bool purge) {
   // Find all installed packages.
   struct package_info* infos = NULL;
   find_packages(&infos, PKG_LOCAL);

   // Find all packags to be purged.
   struct package_info** selected = NULL;
   bool failed = false;
   for (size_t i = 0; i < num_args; ++i) {
      struct package_info* info = find_package_info(&infos, args[i]);
      if (!info) {
         error("Package not found: %s", args[i]);
         failed = true;
      }
      add_package(&selected, info);
      // TODO: handle provided packages
   }
   if (failed)
      return 1;
   failed = false;

   if (!purge) {
      // Check for reverse-dependencies
      for (size_t i = 0; i < buf_len(selected); ++i) {
         struct package* spkg = selected[i]->pkg;
         for (size_t j = 0; j < buf_len(infos); ++j) {
            struct package* pkg = infos[j].pkg;
            for (size_t k = 0; k < buf_len(selected); ++k) {
               if (pkg == selected[k]->pkg)
                  goto next;
            }
            for (size_t k = 0; k < buf_len(pkg->rdepends); ++k) {
               if (!strcmp(spkg->name, pkg->rdepends[k])) {
                  error("Cannot remove %s:%s beacause package %s:%s depends on it.",
                       spkg->name, spkg->version,
                       pkg->name, pkg->version);
                  failed = true;
                  goto next;
               }
            }
         }
      next:;
      }
      if (failed)
         return 1;
   }

   log("Estimating size...");
   log("");
   size_t total_size = 0;
   for (size_t i = 0; i < buf_len(selected); ++i) {
      size_t sz;
      if (pkg_estimate_size(selected[i]->provided_name, &sz)) {
         total_size += sz;
      } else {
         warn("Failed to estimate size for %s", selected[i]->provided_name);
      }
   }

   if (verbosity >= V_NORMAL) {
      print(COLOR_LOG, "Packages (%zu)", buf_len(selected));
      for (size_t i = 0; i < buf_len(selected); ++i) {
         fprintf(stderr, " %s", selected[i]->provider_name);
      }
      fputc('\n', stderr);
   }
   log("");
   const char* unit;
   format_size(&total_size, &unit);
   log("Total Removal Size: %zu%s", total_size, unit);
   log("");
   if (!op_is_set(op, "-y")) {
      if (!yesno("Do you want to %s these packages?", true, purge ? "purge" : "remove"))
      return 1;
      log("");
   }

   for (size_t i = 0; i < buf_len(selected); ++i) {
      const struct package_info* pi = selected[i];
      const struct package* pkg = pi->pkg;
      log("(%zu/%zu) %s %s:%s...",
            i+1, buf_len(selected),
            purge ? "Purging" : "Removing",
            pkg->name, pkg->version);
      if (!purge_package(pkg->name)) {
         error("Failed to purge %s.", pkg->name);
         return 1;
      }
   }

   return 0;

}

