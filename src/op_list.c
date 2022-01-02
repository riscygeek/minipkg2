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
   { "--repo",       false, "List installable packages.",               {NULL}, },
   { "--local",      false, "List installed packages.",                 {NULL}, },
   { "--files",      false, "List the files of an installed package.",  {NULL}, },
   { NULL },
};

defop(list) {
   const bool opt_repo  = op_is_set(op, "--repo");
   const bool opt_local = op_is_set(op, "--local");
   const bool opt_files = op_is_set(op, "--files");

   if (opt_repo && opt_local)
      fail("Either --local or --repo must be set.");
   if (opt_repo && opt_files)
      fail("Option --files is incompatible with option --repo.");
   if (opt_files && num_args == 0)
      fail("Option --files requires at least one argument.");
   if (!opt_files && num_args != 0)
      fail("No arguments for operation info expected.");

   if (opt_files) {
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
   } else {
      struct package_info* pkgs = NULL;
      find_packages(&pkgs, opt_repo ? PKG_REPO : PKG_LOCAL);

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
      return 0;
   }
}
