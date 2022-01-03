#include "minipkg2.h"
#include "cmdline.h"
#include "print.h"
#include "utils.h"

struct cmdline_option clean_options[] = {
   {NULL},
};

defop(clean) {
   (void)op;
   (void)args;
   (void)num_args;
   if (!rm_rf(builddir))
      fail_errno("Failed to clean build directory.");
   return 0;
}

