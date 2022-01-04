#ifndef FILE_MINIPKG2_MINIPKG2_H
#define FILE_MINIPKG2_MINIPKG2_H

#ifndef VERSION
#error "VERSION is not defined"
#endif

#define SHELL  "bash"

extern char* root;
extern char* pkgdir;
extern char* builddir;
extern char* repodir;

void set_root(const char*);
void set_repodir(const char*);

#endif /* FILE_MINIPKG2_MINIPKG2_H */
