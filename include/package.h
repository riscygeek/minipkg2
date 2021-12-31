#ifndef FILE_MINIPKG2_PACKAGE_H
#define FILE_MINIPKG2_PACKAGE_H

struct package {
   char* filepath;

   char* name;
   char* version;
   char* url;
   char* description;
   char** sources;
   char** depends;
};

struct package* parse_package(const char* path);
void free_package(struct package*);
void print_package(const struct package*);

#endif /* FILE_MINIPKG2_PACKAGE_H */
