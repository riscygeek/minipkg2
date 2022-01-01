#include <unistd.h>
#include <fcntl.h>
#include "minipkg2.h"
#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"

struct cmdline_option info_options[] = {
   { "--local",     false, "Select a localy-installed package.",  },
   { "--repo",      false, "Select a repo-package.",              },
   { NULL },
};

defop(info) {
   const bool search_local = op_is_set(op, "--local");
   const bool search_repo  = op_is_set(op, "--repo");
   if (search_local && search_repo)
      fail("Either --local or --repo must be set.");

   const enum package_source src = search_repo ? PKG_REPO : PKG_LOCAL;

   int ec = 0;
   for (size_t i = 0; i < num_args; ++i) {
      const char* end;
      struct package* pkg;
      if ((end = strrchr(args[i], '/')) == NULL) {
         pkg = find_package(args[i], src);
         if (!pkg) {
            error("No such package: %s", args[i]);
            ec = 1;
            continue;
         }
      } else {
         if (!ends_with(args[i], ".mpkg")) {
            error("%s: invalid package format.", end + 1);
            ec = 1;
            continue;
         }
         pkg = parse_package(args[i]);
         if (!pkg) {
            error("%s: failed to parse package.", end + 1);
            ec = 1;
            continue;
         }
      }
      print_package(pkg);
      free_package(pkg);
   }
   return ec;
}
