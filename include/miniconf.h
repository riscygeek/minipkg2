#ifndef FILE_MINIPKG2_MINICONF_H
#define FILE_MINIPKG2_MINICONF_H
#include <stdio.h>
#include "buf.h"

struct miniconf_setting {
   char* name;
   char* value;
};

typedef struct miniconf {
   struct miniconf_setting* settings;
} miniconf_t;

miniconf_t* miniconf_parse(FILE* file, char** errmsg);
const char* miniconf_get(const miniconf_t* conf, const char* name);
void miniconf_dump(const miniconf_t* conf, FILE* file);
void miniconf_free(miniconf_t* conf);

#endif /* FILE_MINIPKG2_MINICONF_H */
