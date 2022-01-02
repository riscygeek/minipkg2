#ifndef FILE_MINIPKG2_MINIPKG2_H
#define FILE_MINIPKG2_MINIPKG2_H

#define VERSION "0.1"
#define SHELL  "bash"

extern char* root;
extern char* pkgdir;
extern char* builddir;
extern char* repodir;
extern int   verbosity;

void set_root(const char*);
void set_repodir(const char*);

#endif /* FILE_MINIPKG2_MINIPKG2_H */
