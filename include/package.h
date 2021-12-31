#ifndef FILE_MINIPKG2_PACKAGE_H
#define FILE_MINIPKG2_PACKAGE_H
#include <stdbool.h>

enum package_source {
   PKG_LOCAL,
   PKG_REPO,
};

struct package {
   char* filepath;

   char* name;
   char* version;
   char* url;
   char* description;
   char** sources;
   char** depends;
};

struct package_info {
   char* provided_name;
   char* provider_name;
   struct package* pkg;
   bool is_provided;
};

struct package* parse_package(const char* path);
void free_package(struct package*);
void print_package(const struct package*);
struct package* find_package(const char* name, enum package_source src);
bool find_packages(struct package_info**, enum package_source src);
void free_package_infos(struct package_info**);
int pkg_info_cmp(const void* p1, const void* p2);

#endif /* FILE_MINIPKG2_PACKAGE_H */
