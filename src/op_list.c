#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include "minipkg2.h"
#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"
#include "buf.h"


struct cmdline_option list_options[] = {
   { "--repo",       OPT_BASIC, "List installable packages.",               {NULL}, },
   { "--local",      OPT_BASIC, "List installed packages.",                 {NULL}, },
   { "--files",      OPT_BASIC, "List the files of an installed package.",  {NULL}, },
   { "--upgradable", OPT_BASIC, "List upgradable packages.",                {NULL}, },
   { NULL },
};

defop(list) {
   int num_optset = 0;
   for (size_t i = 0; list_options[i].option; ++i) {
      if (list_options[i].selected)
         ++num_optset;
   }

   // By default assume --local
   if (num_optset == 0) {
      op_get_opt(op, "--local")->selected = true;
   } else if (num_optset > 1) {
      error("Only one list options is allowed.");
      return 1;
   }

   if (op_is_set(op, "--files")) {
      int ec = 0;
      for (size_t i = 0; i < num_args; ++i) {
         const char* name = args[i];
         if (strcont(name, '/')) {
            error("Invalid package name: %s", name);
            ec = 1;
            continue;
         }
         char* path = xstrcatl(pkgdir, "/", name, "/files");
         char* files = read_file(path);
         if (files) {
            printf("%s", files);
         } else {
            error("Package not found: %s", name);
         }
         free(path);
         free(files);
      }
      return ec;
   } else if (op_is_set(op, "--local") || op_is_set(op, "--repo")) {
      struct package_info* pkgs = NULL;
      find_packages(&pkgs, op_is_set(op, "--repo") ? PKG_REPO : PKG_LOCAL);

      for (size_t i = 0; i < buf_len(pkgs); ++i) {
         const struct package_info* info = &pkgs[i];
         const struct package* pkg = info->pkg;
         if (!pkg)
            continue;

         if (info->is_provided) {
            printf("%s %s (provided by %s)\n", info->provided_name, pkg->version, pkg->name);
         } else {
            printf("%s %s\n", pkg->name, pkg->version);
         }
      }
      free_package_infos(&pkgs);
   } else if (op_is_set(op, "--upgradable")) {
      struct package_info* pkgs_repo = NULL;
      struct package_info* pkgs_local = NULL;

      find_packages(&pkgs_repo, PKG_REPO);
      find_packages(&pkgs_local, PKG_LOCAL);

      for (size_t i = 0; i < buf_len(pkgs_local); ++i) {
         struct package* lp = pkgs_local[i].pkg;
         for (size_t j = 0; j < buf_len(pkgs_repo); ++j) {
            struct package* rp = pkgs_repo[j].pkg;
            if (!strcmp(lp->name, rp->name)) {
               if (strcmp(lp->version, rp->version) != 0) {
                  printf("%s local=%s repo=%s\n", lp->name, lp->version, rp->version);
               }
               break;
            }
         }
      }
      free_package_infos(&pkgs_repo);
      free_package_infos(&pkgs_local);
   }
   return 0;
}
