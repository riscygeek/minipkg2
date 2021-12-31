#include "cmdline.h"
#include "config.h"

const char* root = "/";

int main(int argc, char* argv[]) {
   return parse_cmdline(argc, argv);
}
