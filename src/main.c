#include "minipkg2.h"
#include "cmdline.h"

int main(int argc, char* argv[]) {
   set_root("/");
   return parse_cmdline(argc, argv);
}
