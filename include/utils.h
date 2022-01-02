#ifndef FILE_MINIPKG2_UTILS_H
#define FILE_MINIPKG2_UTILS_H
#include <sys/stat.h>
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
char* xstrcatl_impl(const char*, const char*, ...);
char* freadline(FILE*);
char** freadlines(FILE*);
char* read_file(const char*);
char* fread_file(FILE*);
char* xreadlink(const char*);
bool  ends_with(const char*, const char*);
bool  download(const char* url, const char* dest, bool overwrite);
bool  mkparentdirs(const char*, mode_t);
bool  mkdir_p(const char*, mode_t);
bool  rm_rf(const char*);
bool  copy_file(const char* source, const char* dest);
bool  create_archive(const char* file, const char* dir);
int   waitexit(pid_t pid, int limit);
bool  write_file(const char* filename, const char* data);
bool  print_file(FILE* to, const char* filename);
void  redir_file(FILE* from, FILE* to);
void  format_size(size_t* sz, const char** unit);
bool  is_symlink(const char*);
bool  dir_is_empty(const char*);
bool  xstreql_impl(const char*, ...);

#define isname0(ch)           (isalpha(ch) || (ch) == '_')
#define isname(ch)            (isalnum(ch) || (ch) == '_')
#define strcont(str, ch)      (strchr(str, ch) != NULL)
#define starts_with(s1, s2)   (!strncmp((s1), (s2), strlen(s2)))
#define xstrcatl(...)         xstrcatl_impl(__VA_ARGS__, NULL)
#define xstreql(...)          xstreql_impl(__VA_ARGS__, NULL)

#endif /* FILE_MINIPKG2_UTILS_H */
