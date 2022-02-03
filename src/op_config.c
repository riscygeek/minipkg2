#include "minipkg2.h"
#include "miniconf.h"
#include "cmdline.h"
#include "utils.h"
#include "print.h"

struct cmdline_option config_options[] = {
   { "--dump",          OPT_BASIC, "Parse and print the config file.",              {NULL}, },
   {NULL},
};

defop(config) {
    (void)args;
    (void)num_args;
    const bool opt_dump = op_is_set(op, "--dump");


    if (opt_dump) {
        char* path = xstrcat(root, "/etc/minipkg2.conf");
        FILE* file = fopen(path, "r");
        if (!file) {
            error_errno("config: Cannot open '%s'", path);
            free(path);
            return 1;
        }
        free(path);

        char* errmsg;
        miniconf_t* config = miniconf_parse(file, &errmsg);

        if (!config) {
            error("config: Failed to parse config: %s", errmsg);
            miniconf_free(config);
            return 1;
        }

        miniconf_dump(config, stdout);
        miniconf_free(config);
        return 0;
    } else {
        error("config: No options specified.");
        return 1;
    }
}
