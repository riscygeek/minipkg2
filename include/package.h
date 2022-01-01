#ifndef FILE_MINIPKG2_PACKAGE_H
#define FILE_MINIPKG2_PACKAGE_H
#include <stdbool.h>

enum package_source {
   PKG_LOCAL,
   PKG_REPO,
};

struct package {
   char* pkgfile;
   char* name;
   char* version;
   char* url;
   char* description;
   char** sources;
   char** depends;
   char** provides;
   char** conflicts;
};

struct package_info {
   char* provided_name;
   char* provider_name;
   struct package* pkg;
   bool is_provided;
};

struct package* parse_package(const char* path);

void print_package(const struct package*);

struct package* find_package(const char* name, enum package_source src);
bool find_packages(struct package_info**, enum package_source src);
struct package_info* find_package_info(struct package_info* const*, const char* pkg);

void free_package(struct package*);
void free_package_infos(struct package_info**);

bool pkg_is_installed(const char* name);
bool pkg_build(struct package* pkg, const char* bmpkg, bool verbose);
bool binpkg_install(const char* binpkg);

#endif /* FILE_MINIPKG2_PACKAGE_H */
