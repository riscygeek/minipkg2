#include <limits.h>
#include <unistd.h>
#include "minipkg2.h"
#include "cmdline.h"
#include "package.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

struct cmdline_option build_options[] = {
   { "-y",              OPT_BASIC, "Don't ask for confirmation.",             {NULL}, },
   { "--yes",           OPT_ALIAS, NULL,                                      {"-y"}, },
   { "--clean-build",   OPT_BASIC, "Perform a clean build.",                  {NULL}, },
   { "--bindir",        OPT_ARG,   "Specify the binary package directory.",   {NULL}, },
   {NULL},
};

defop(build) {
   struct package_info* infos = NULL;
   find_packages(&infos, PKG_REPO);

   log("Resolving build dependencies...");

   struct package** pkgs = NULL;
   bool success = true;
   for (size_t i = 0; i < num_args; ++i) {
      struct package_info* info = find_package_info(&infos, args[i]);
      if (!info) {
         error("Package not found: %s", args[i]);
         success = false;
         continue;
      }
      struct package* pkg = info->pkg;
      buf_push(pkgs, pkg);
      for (size_t j = 0; j < buf_len(pkg->bdepends); ++j) {
         const char* bdep = pkg->bdepends[j];
         if (!pkg_is_installed(bdep)) {
            error("Package '%s' has unmet build dependency '%s'", pkg->name, bdep);
            success = false;
            continue;
         }
      }
   }
   if (!success) {
      free_package_infos(&infos);
      buf_free(pkgs);
      return 1;
   }

   log("Running prebuild checks...");
   for (size_t i = 0; i < buf_len(pkgs); ++i) {
      success &= pkg_prebuild_checks(pkgs[i]);
   }

   if (!success) {
      free_package_infos(&infos);
      buf_free(pkgs);
      return 1;
   }

   log("");
   if (verbosity >= V_NORMAL) {
      print(COLOR_LOG, "Packages (%zu)", buf_len(pkgs));

      for (size_t i = 0; i < buf_len(pkgs); ++i) {
         print(0, " %s", pkgs[i]->name);
      }
      println(0, "");
      log("");
   }

   if (!op_is_set(op, "-y")) {
      if (!yesno("Proceed with installation?", true)) {
         free_package_infos(&infos);
         buf_free(pkgs);
         return 1;
      }
      log("");
   }

   log("Downloading sources...");

   const size_t num_pkgs = buf_len(pkgs);
   success = true;
   for (size_t i = 0; i < num_pkgs; ++i) {
      log("(%zu/%zu) Downloading %s:%s...",
            i+1, num_pkgs,
            pkgs[i]->name, pkgs[i]->version);
      if (!pkg_download_sources(pkgs[i]))
         success = false;
   }

   if (!success) {
      free_package_infos(&infos);
      buf_free(pkgs);
      return 1;
   }

   log("");
   log("Building packages...");

   char* binpkgdir = op_is_set(op, "--bindir") ? xstrdup(op_get_arg(op, "--bindir")) : getcwd(xmalloc(PATH_MAX), PATH_MAX);
   
   for (size_t i = 0; i < buf_len(pkgs); ++i) {
      struct package* pkg = pkgs[i];
      log("(%zu/%zu) Building %s:%s...",
            i+1, num_pkgs,
            pkg->name, pkg->version);
      char* binpkg = xstrcatl(binpkgdir, "/", pkg->name, ":", pkg->version, ".bmpkg.tar.gz");
      char* filesdir = xstrcatl(repodir, "/", pkg->name, "/files");

      success = pkg_build(pkg, binpkg, filesdir);

      free(filesdir);
      free(binpkg);

      if (!success) {
         error("Failed to build '%s:%s'", pkg->name, pkg->version);
      }
   }

   free(binpkgdir);
   free_package_infos(&infos);
   buf_free(pkgs);
   return success ? 0 : 1;
}
