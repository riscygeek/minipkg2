#include "minipkg2.h"
#include "package.h"
#include "cmdline.h"
#include "utils.h"

struct cmdline_option install_options[] = {
   { NULL },
};

static void find_dependencies(struct package** pkgs, const struct package* pkg) {
}

defop(install) {
   struct package** pkgs = NULL;
   for (size_t i = 0; i < num_args; ++i) {

   }
   return 0;
}
