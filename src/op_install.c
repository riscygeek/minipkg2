#include <unistd.h>
#include "minipkg2.h"
#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"
#include "buf.h"

struct cmdline_option install_options[] = {
   { "-y",              OPT_BASIC, "Don't ask for confirmation.",    {NULL}, },
   { "--yes",           OPT_ALIAS, NULL,                             {"-y"}, },
   { "--clean-build",   OPT_BASIC, "Perform a clean build.",         {NULL}, },
   { "--no-deps",       OPT_BASIC, "Don't check for dependencies.",  {NULL}, },
   { "-s",              OPT_BASIC, "Skip installed packages.",       {NULL}, },
   { "--skip-installed",OPT_BASIC, NULL,                             {"-s"}, },
   { NULL },
};

struct package_install_info {
   struct package* pkg;
   char** remove_pkgs;
};

defop(install) {
   // TODO: add support for non-repo packages

   struct package_info* infos = NULL;
   find_packages(&infos, PKG_REPO);
   const bool opt_no_deps = op_is_set(op, "--no-deps");
   const bool opt_skip = op_is_set(op, "-s");

   if (!opt_no_deps)
      log("Resolving dependencies...");


   const char** binpkgs = NULL;
   struct package** pkgs = NULL;
   for (size_t i = 0; i < num_args; ++i) {
      if (strcont(args[i], '/') || strcont(args[i], '.')) {
         // If this is a file
         if (ends_with(args[i], ".mpkg")) {
            struct package* pkg = parse_package(args[i]);
            if (!pkg)
               fail("Failed to parse '%s'", args[i]);
            add_package(&pkgs, pkg, !opt_skip);
         } else if (ends_with(args[i], ".bmpkg.tar.gz")) {
            if (access(args[i], F_OK) != 0)
               fail("No such file '%s'", args[i]);
            buf_push(binpkgs, args[i]);
         } else {
            fail("Invalid file format of '%s'.", args[i]);
         }
      } else {
         struct package_info* info = find_package_info(&infos, args[i]);
         if (!info || !info->pkg)
            fail("Package %s not found", args[i]);
         if (!opt_no_deps)
            find_dependencies(&pkgs, &infos, info->pkg, false);
         add_package(&pkgs, info->pkg, !opt_skip);
      }
   }

   if (buf_len(pkgs) != 0 && buf_len(binpkgs) != 0)
      fail("Mixing the installation of binary packages and normal packages is not supported.");

   if (!buf_len(pkgs) && !buf_len(binpkgs)) {
      warn("No packages to install.");
      return 0;
   }

   // TODO: handle conflicts and provides

   // Check for package conflicts.
   bool success = true;
   struct package_info* installed_pkgs = NULL;
   struct package_install_info* install_pkgs = NULL;
   if (find_packages(&installed_pkgs, PKG_LOCAL)) {
      for (size_t i = 0; i < buf_len(pkgs); ++i) {
         struct package* pkg = pkgs[i];
         struct package_install_info iinfo;
         iinfo.pkg = pkg;
         iinfo.remove_pkgs = NULL;
         for (size_t j = 0; j < buf_len(pkg->conflicts); ++j) {
            // Search for packages to be installed.
            const struct package_info* conflict = find_package_info(&infos, pkg->conflicts[j]);
            if (conflict != NULL) {
               error("Package '%s' conflicting with package '%s'.", pkg->name, conflict->pkg->name);
               success = false;
            }

            // Search for already installed packages.
            conflict = find_package_info(&installed_pkgs, pkg->conflicts[j]);
            if (conflict != NULL) {
               if (!strlist_contains(pkg->provides, conflict->pkg->name)) {
                  error("Package '%s' conflits with installed package '%s'.", pkg->name, conflict->pkg->name);
                  success = false;
               } else {
                  buf_push(iinfo.remove_pkgs, xstrdup(conflict->pkg->name));
               }
            }
         }
         buf_push(install_pkgs, iinfo);
      }
      for (size_t i = 0; i < buf_len(installed_pkgs); ++i) {
         const struct package* ipkg = installed_pkgs[i].pkg;
         for (size_t j = 0; j < buf_len(ipkg->conflicts); ++j) {
            for (size_t k = 0; k < buf_len(install_pkgs); ++k) {
               if (!strcmp(ipkg->conflicts[j], install_pkgs[k].pkg->name)) {
                  if (!strlist_contains(ipkg->provides, install_pkgs[k].pkg->name)) {
                     error("Package '%s' conflicts with installed package '%s'.",
                           install_pkgs[k].pkg->name, ipkg->name);
                     success = false;
                  } else {
                     if (!strlist_contains(install_pkgs[k].remove_pkgs, ipkg->name))
                        buf_push(install_pkgs[k].remove_pkgs, xstrdup(ipkg->name));
                  }
               }
            }
         }
      }
      free_package_infos(&installed_pkgs);
   } else {
      for (size_t i = 0; i < buf_len(pkgs); ++i) {
         struct package_install_info iinfo;
         iinfo.pkg = pkgs[i];
         iinfo.remove_pkgs = NULL;
         buf_push(install_pkgs, iinfo);
      }
   }

   if (!success)
      return 1;

   log("");
   if (verbosity >= V_NORMAL) {
      print(COLOR_LOG, "Packages (%zu)", buf_len(install_pkgs) + buf_len(binpkgs));

      for (size_t i = 0; i < buf_len(pkgs); ++i) {
         print(0, " %s", install_pkgs[i].pkg->name);
      }
      for (size_t i = 0; i < buf_len(binpkgs); ++i) {
         print(0, " %s", binpkgs[i]);
      }
      println(0, "");
      log("");
   }

   if (!op_is_set(op, "-y")) {
      if (!yesno("Proceed with installation?", true))
         return 1;
      log("");
   }

   if (op_is_set(op, "--clean-build")) {
      log("Cleanig the build directories...");
      for (size_t i = 0; i < buf_len(install_pkgs); ++i) {
         struct package* p = install_pkgs[i].pkg;
         char* dir = xstrcatl(builddir, "/", p->name, "-", p->version);
         rm_rf(dir);
         free(dir);
      }
   }

   log("Downloading sources...");

   const size_t num_pkgs = buf_len(install_pkgs);
   success = true;
   for (size_t i = 0; i < num_pkgs; ++i) {
      log("(%zu/%zu) Downloading %s:%s...",
            i+1, num_pkgs,
            install_pkgs[i].pkg->name, install_pkgs[i].pkg->version);
      if (!pkg_download_sources(install_pkgs[i].pkg))
         success = false;
   }

   if (!success)
      return 1;

   log("");
   log("Processing packages...");

   for (size_t i = 0; i < buf_len(binpkgs); ++i) {
      log("(%zu/%zu) Installing %s...",
            i+1, buf_len(binpkgs),
            binpkgs[i]);
      if (!binpkg_install(binpkgs[i]))
         return 1;
   }

   for (size_t i = 0; i < num_pkgs; ++i) {
      struct package* pkg = install_pkgs[i].pkg;
      log("(%zu/%zu) Building %s:%s...",
            i+1, num_pkgs,
            pkg->name, pkg->version);
      char* binpkg = xstrcatl(builddir, "/", pkg->name, "-", pkg->version, "/", pkg->name, ":", pkg->version, ".bmpkg.tar.gz");
      char* filesdir = xstrcatl(repodir, "/", pkg->name, "/files");

      // Build the package.
      success = pkg_build(pkg, binpkg, filesdir);

      free(filesdir);

      if (!success)
         return free(binpkg), 1;

      // Remove conflicting packages.
      for (size_t j = 0; j < buf_len(install_pkgs[i].remove_pkgs); ++j) {
         log(" (%zu/%zu) Removing %s...", j+1, buf_len(install_pkgs[i].remove_pkgs),
             install_pkgs[i].remove_pkgs[j]);
         purge_package(install_pkgs[i].remove_pkgs[j]);
      }

      log("(%zu/%zu) Installing %s:%s...",
            i+1, num_pkgs,
            pkg->name, pkg->version);

      // binpkg_install
      binpkg_install(binpkg);
      free(binpkg);

   }

   free_package_infos(&infos);

   return 0;
}
