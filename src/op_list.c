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


struct cmdline_option list_options[] = {
   { "--repo",       false,    },
   { "--local",      false,    },
   { "--files",      false,    },
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
         char* path = xstrcatl(pkgdir, "/", name, "/files", NULL);
         char* files = read_file(path);
         if (files) {
            puts(files);
         } else {
            error("Package not found: %s", name);
         }
         free(path);
         free(files);
      }
      return ec;
   } else {
      DIR* dir = opendir(opt_repo ? repodir : pkgdir);
      if (!dir) {
         if (errno == ENOENT)
            return 0;
         fail_errno("Failed to open '%s'", pkgdir);
      }

      struct dirent* ent;
      while ((ent = readdir(dir)) != NULL) {
         if (ent->d_name[0] == '.')
            continue;
         const char* name = ent->d_name;
         char* path;
         if (opt_repo) {
            path = xstrcatl(repodir, "/", name, "/package.build", NULL);
         } else {
            path = xstrcatl(pkgdir, "/", name, "/package.info", NULL);
         }
         struct package* pkg = parse_package(path);
         free(path);
         if (!pkg)
            fail("Failed to parse package '%s'", name);

         if (!strcmp(name, pkg->name)) {
            printf("%s %s\n", name, pkg->version);
         } else {
            printf("%s %s (provided by %s)\n", name, pkg->version, pkg->name);
         }
         free_package(pkg);
      }
      closedir(dir);
      return 0;
   }
}
