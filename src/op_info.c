#include "package.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"

defop(info) {
   for (size_t i = 0; i < num_args; ++i) {
      const char* end;
      if ((end = strrchr(args[i], '/')) == NULL) {
         fail("Not supported.");
      } else {
         if (!ends_with(args[i], ".mpkg")) {
            fail("%s: invalid package format.", end + 1);
         }
         struct package* pkg = parse_package(args[i]);
         if (!pkg) {
            fail("%s: failed to parse package.", end + 1);
         }
         print_package(pkg);
         free_package(pkg);
      }
   }
}
