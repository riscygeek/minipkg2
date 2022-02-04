#ifndef FILE_MINIPKG2_MINIPKG2_H
#define FILE_MINIPKG2_MINIPKG2_H

#ifndef VERSION
#error "VERSION is not defined"
#endif

#ifndef CONFIG_PREFIX
#error "CONFIG_PREFIX is not defined"
#endif

#ifndef CONFIG_SYSCONFDIR
#error "CONFIG_SYSCONFDIR is not defined"
#endif

#ifndef CONFIG_LIBDIR
#error "CONFIG_LIBDIR is not defined"
#endif

#define CONFIG_FILE CONFIG_PREFIX "/" CONFIG_SYSCONFDIR "/minipkg2.conf"
#define ENV_FILE    CONFIG_PREFIX "/" CONFIG_LIBDIR     "/minipkg2/env.bash"

#define SHELL  "bash"

extern char* root;
extern char* pkgdir;
extern char* builddir;
extern char* repodir;
extern const char* host;
extern const char* jobs;
extern char* self;

void set_root(const char*);
void set_repodir(const char*);
void init_self(void);
void print_version(void);

#endif /* FILE_MINIPKG2_MINIPKG2_H */
