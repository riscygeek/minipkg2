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
   { NULL },
};

static void add_package(struct package*** pkgs, struct package* pkg, bool force) {
   if (!force && pkg_is_installed(pkg->name))
      return;
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
      add_package(pkgs, dep->pkg, false);
   }
}


defop(install) {
   // TODO: add support for non-repo packages

   struct package_info* infos = NULL;
   find_packages(&infos, PKG_REPO);
   const bool opt_no_deps = op_is_set(op, "--no-deps");

   if (!opt_no_deps)
      log("Resolving dependencies...");


   struct package** pkgs = NULL;
   for (size_t i = 0; i < num_args; ++i) {
      struct package_info* info = find_package_info(&infos, args[i]);
      if (!info || !info->pkg)
         fail("Package %s not found", args[i]);
      if (!opt_no_deps)
         find_dependencies(&pkgs, &infos, info->pkg);
      add_package(&pkgs, info->pkg, true);
   }

   // TODO: handle conflicts and provides

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
      if (!yesno("Proceed with installation?", true))
         return 1;
      log("");
   }

   if (op_is_set(op, "--clean-build")) {
      log("Cleanig the build directories...");
      for (size_t i = 0; i < buf_len(pkgs); ++i) {
         struct package* p = pkgs[i];
         char* dir = xstrcatl(builddir, "/", p->name, "-", p->version);
         rm_rf(dir);
         free(dir);
      }
   }

   log("Downloading sources...");

   const size_t num_pkgs = buf_len(pkgs);
   bool success = true;
   for (size_t i = 0; i < num_pkgs; ++i) {
      log("(%zu/%zu) Downloading %s:%s...",
            i+1, num_pkgs,
            pkgs[i]->name, pkgs[i]->version);
      if (!pkg_download_sources(pkgs[i]))
         success = false;
   }

   if (!success)
      return 1;

   log("");
   log("Processing packages...");

   for (size_t i = 0; i < num_pkgs; ++i) {
      struct package* pkg = pkgs[i];
      log("(%zu/%zu) Building %s:%s...",
            i+1, num_pkgs,
            pkg->name, pkg->version);
      char* binpkg = xstrcatl(builddir, "/", pkg->name, "-", pkg->version, "/", pkg->name, ":", pkg->version, ".tar.gz");
      char* filesdir = xstrcatl(repodir, "/", pkg->name, "/files");

      // Build the package.
      success = pkg_build(pkg, binpkg, filesdir);

      free(filesdir);

      if (!success)
         return free(binpkg), 1;

      log("(%zu/%zu) Installing %s:%s...",
            i+1, num_pkgs,
            pkg->name, pkg->version);

      // binpkg_install
      binpkg_install(binpkg);
      free(binpkg);

   }

   return 0;
}
