#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "minipkg2.h"
#include "cmdline.h"
#include "package.h"
#include "print.h"
#include "utils.h"
#include "buf.h"


struct cmdline_option check_options[] = {
   { "--syntax",        OPT_BASIC, "Check the syntax of .mpkg build files.",   {NULL}, },
   { "--files",         OPT_BASIC, "Check the consistency of a local package.",{NULL}, },
   { "--system",        OPT_BASIC, "Same as --files, but checks all packages.",{NULL}, },
   { "--full",          OPT_BASIC, "Perform a full system check.",             {NULL}, },
   { "--werror",        OPT_BASIC, "Treat warnings as errors.",                {NULL}, },
   { NULL }
};

static bool check_files(const char* pkgname, int color) {
    bool success = true;
    char** files = lpkg_get_files(pkgname);
    struct stat st;

    for (size_t i = 0; i < buf_len(files); ++i) {
        char* path = xstrcatl(root, "/", files[i]);
        if (lstat(path, &st) != 0) {
            println(color, "%s: File '%s' does not exist.", pkgname, files[i]);
            success = false;
        }
        free(path);
    }

    free_strlist(&files);
    return success;
}

defop(check) {
    const bool opt_syntax = op_is_set(op, "--syntax");
    const bool opt_files  = op_is_set(op, "--files");
    const bool opt_system = op_is_set(op, "--system");
    const bool opt_full   = op_is_set(op, "--full");
    const bool opt_werror = op_is_set(op, "--werror");
    const int color = opt_werror ? COLOR_ERROR : COLOR_WARN;

    if ((opt_syntax + opt_files + opt_system + opt_full) > 1) {
        error("check: More than one run-option specified.");
        return 1;
    }

    bool success = true;

    if (opt_files) {
        struct package_info* pkgs = NULL;
        find_packages(&pkgs, PKG_LOCAL);
        for (size_t i = 0; i < num_args; ++i) {
            struct package_info* info = find_package_info(&pkgs, args[i]);
            if (!info) {
                error("No such package: %s", args[i]);
                success = false;
                continue;
            }
            logv(V_VERBOSE, "Checking package '%s:%s'...", info->pkg->name, info->pkg->version);

            const bool tmp = check_files(info->pkg->name, color);
            if (opt_werror)
                success &= tmp;
        }
        free_package_infos(&pkgs);
    } else if (opt_system) {
        if (num_args > 0) {
            error("check: Option '--system' doesn't expect any arguments.");
            return 1;
        }

        struct package_info* pkgs = NULL;
        find_packages(&pkgs, PKG_LOCAL);

        for (size_t i = 0; i < buf_len(pkgs); ++i) {
            const struct package* pkg = pkgs[i].pkg;
            logv(V_VERBOSE, "Checking package '%s:%s'...", pkg->name, pkg->version);
            const bool tmp = check_files(pkg->name, color);
            if (opt_werror)
                success &= tmp;
        }

        free_package_infos(&pkgs);
    } else if (opt_syntax) {
        error("check: Option '--syntax' is not yet supported.");
        return 1;
    } else if (opt_full) {
        error("check: Option '--full' is not yet supported.");
        return 1;
    } else {
        error("check: No option given.");
        return 1;
    }
    return !success;
}
