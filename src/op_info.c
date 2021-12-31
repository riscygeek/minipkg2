#include <unistd.h>
#include <fcntl.h>
#include "minipkg2.h"
#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"

struct cmdline_option info_options[] = {
   { "--local",     false,    },
   { "--repo",      false,    },
   { NULL },
};

defop(info) {
   const bool search_local = op_is_set(op, "--local");
   const bool search_repo  = op_is_set(op, "--repo");
   if (search_local && search_repo)
      fail("Either --local or --repo must be set.");

   int ec = 0;
   for (size_t i = 0; i < num_args; ++i) {
      char* path;
      const char* end;
      if ((end = strrchr(args[i], '/')) == NULL) {
         if (search_local) {
            path = xstrcatl(pkgdir, "/", args[i], ".mpkg", NULL);
         } else if (search_repo) {
            path = xstrcatl(repodir, "/", args[i], ".mpkg", NULL);
         } else {
            path = xstrcatl(pkgdir, "/", args[i], ".mpkg", NULL);
            if (access(path, O_RDONLY) != 0)
               path = xstrcatl(repodir, "/", args[i], ".mpkg", NULL);
         }
         if (access(path, O_RDONLY) != 0) {
            error("Package not found: %s", args[i]);
            ec = 1;
            continue;
         }
      } else {
         if (!ends_with(args[i], ".mpkg")) {
            error("%s: invalid package format.", end + 1);
            ec = 1;
            continue;
         }
         path = args[i];
      }
      struct package* pkg = parse_package(path);
      if (!pkg) {
         error("%s: failed to parse package.", end + 1);
         ec = 1;
         continue;
      }
      print_package(pkg);
      free_package(pkg);
   }
   return ec;
}
