#ifndef FILE_MINIPKG2_UTILS_H
#define FILE_MINIPKG2_UTILS_H
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define new(type) (type*)xmalloc(sizeof(type))
#define arraylen(a) (sizeof(a) / sizeof(a[0]))

#if __GNUC__
#define likely(expr)    __builtin_expect(!!(expr), 1)
#define unlikely(expr)  __builtin_expect(!!(expr), 0)
#else
#define likely(expr)
#define unlikely(expr)
#endif

void* xmalloc(size_t);
char* xstrdup(const char*);
char* xstrcat(const char*, const char*);
char* xstrcatl(const char*, const char*, ...);
char* freadline(FILE*);
char* read_file(const char*);
char* xreadlink(const char*);
bool  ends_with(const char*, const char*);

#define isname0(ch)           (isalpha(ch) || (ch) == '_')
#define isname(ch)            (isalnum(ch) || (ch) == '_')
#define strcont(str, ch)      (strchr(str, ch) != NULL)
#define starts_with(s1, s2)   (!strncmp((s1), (s2), strlen(s2)))

#endif /* FILE_MINIPKG2_UTILS_H */
