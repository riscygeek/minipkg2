#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include "minipkg2.h"
#include "package.h"
#include "print.h"
#include "utils.h"
#include "buf.h"

static bool is_empty(const char* s) {
   return !s || !s[0];
}


struct package* parse_package(const char* path) {
   // Check if `path` can be read.
   if (access(path, O_RDONLY) != 0) {
      error_errno("Cannot access '%s'", path);
      return NULL;
   }

   struct package* pkg = new(struct package);
   
   // The shell script that will be run.
   static const char shell_script[] =
      "[[ -f $ROOT/etc/minipkg2.conf ]] && source \"$ROOT/etc/minipkg2.conf\"\n"
      "source \"$pkgfile\"\n"
      "echo \"$pkgname\"\n"
      "echo \"$pkgver\"\n"
      "echo \"$url\"\n"
      "echo \"$description\"\n"
      "for src in \"${sources[@]}\"; do\n"
      "  echo \"${sources[@]}\"\n"
      "done\n"
      "echo\n"
      "for dep in \"${depends[@]}\"; do\n"
      "  echo \"${depends[@]}\"\n"
      "done\n"
      "echo\n"
      "exit 0\n"
      ;

   // pipefd[0] : minipkg2 -> bash
   // pipefd[1] : minipkg2 <- bash
   int pipefd[2][2];

   check0(pipe(pipefd[0]));
   check0(pipe(pipefd[1]));

   // Write the shell-script to the write-pipe and close it.
   check(write(pipefd[0][1], shell_script, sizeof(shell_script) - 1), == (sizeof(shell_script) - 1));

   // TODO: use posix_spawn() instead.

   pid_t pid;
   check(pid = vfork(), >= 0);

   if (pid == 0) {
      setenv("ROOT", root, 1);
      setenv("pkgfile", path, 1);

      // Close unused pipes.
      close(pipefd[1][0]);
      close(pipefd[0][1]);

      // Remap stdin.
      check0f(close(STDIN_FILENO));
      checkf(dup(pipefd[0][0]), == STDIN_FILENO);

      // Remap stdout.
      check0f(close(STDOUT_FILENO));
      checkf(dup(pipefd[1][1]), == STDOUT_FILENO);

      // Remap stderr.
      check0f(close(STDERR_FILENO));
      checkf(open("/dev/null", O_RDONLY), == STDERR_FILENO);

      // Run the shell (by default bash).
      execlp(SHELL, SHELL, NULL);
      _exit(1);
   } else {
      // Close unused pipes
      close(pipefd[0][0]);
      close(pipefd[0][1]);
      close(pipefd[1][1]);

      // Wait for the program to exit.
      const int limit = 10;
      int cnt = 0;
      int wstatus;
      do {
         waitpid(pid, &wstatus, 0);
         if (++cnt == limit)
            fail_errno("Failed to wait for sub-process");
      } while (!WIFEXITED(wstatus));

      if (WEXITSTATUS(wstatus) != 0) {
         fail("Sub-process exited with %d", WEXITSTATUS(wstatus));
      }
      
      FILE* file;
      check(file = fdopen(pipefd[1][0], "r"), != NULL);

      // Read the name, version, url & description.
      pkg->name         = freadline(file);
      pkg->version      = freadline(file);
      pkg->url          = freadline(file);
      pkg->description  = freadline(file);

      pkg->sources = NULL;
      pkg->depends = NULL;

      char* line;

      // Read `sources` until an empty line is read.
      while (!is_empty(line = freadline(file)))
         buf_push(pkg->sources, line);
      free(line);

      // Read `depends` until an empty line is read.
      while (!is_empty(line = freadline(file)))
         buf_push(pkg->depends, line);
      free(line);

      fclose(file);
   }

   return pkg;
}
void free_package(struct package* pkg) {
   free(pkg->filepath);
   free(pkg->name);
   free(pkg->version);
   free(pkg->url);
   free(pkg->description);
   for (size_t i = 0; i < buf_len(pkg->sources); ++i) {
      free(pkg->sources[i]);
   }
   buf_free(pkg->sources);
   for (size_t i = 0; i < buf_len(pkg->depends); ++i) {
      free(pkg->depends[i]);
   }
   buf_free(pkg->depends);
}

static void print_line(const char* first, size_t num, char* const * strs) {
   printf("%s%*s:", first, 30 - (int)strlen(first), "");
   for (size_t i = 0; i < num; ++i) {
      printf(" %s", strs[i]);
   }
   putchar('\n');
}
void print_package(const struct package* pkg) {
   print_line("Name",         1, &pkg->name);
   print_line("Version",      1, &pkg->version);
   print_line("URL",          1, &pkg->url);
   print_line("Description",  1, &pkg->description);
   print_line("Sources",      buf_len(pkg->sources), pkg->sources);
   print_line("Dependencies", buf_len(pkg->depends), pkg->depends);
}
struct package* find_package(const char* name, enum package_source src) {
   char* path;
   switch (src) {
   case PKG_LOCAL:
      path = xstrcatl(pkgdir, "/", name, "/package.info", NULL);
      break;
   case PKG_REPO:
      path = xstrcatl(repodir, "/", name, "/package.build", NULL);
      break;
   default:
      fail("find_package(): Invalid package source %d", src);
   }
   struct package* pkg = (access(path, O_RDONLY) == 0) ? parse_package(path) : NULL;
   free(path);
   return pkg;
}

static int pkg_info_cmp(const void* p1, const void* p2) {
   const struct package_info* i1 = p1;
   const struct package_info* i2 = p2;
   return strcmp(i1->provided_name, i2->provided_name);
}
bool find_packages(struct package_info** pkgs, enum package_source src) {
   const char* base_dir;
   const char* suffix;
   switch (src) {
   case PKG_LOCAL:
      base_dir = pkgdir;
      suffix = "/package.info";
      break;
   case PKG_REPO:
      base_dir = repodir;
      suffix = "/package.build";
      break;
   default:
      fail("find_packages(): Invalid package source %d", src);
   }
   DIR* dir = opendir(base_dir);
   if (!dir) {
      if (errno == ENOENT)
         return false;
      fail_errno("Failed to open '%s'", base_dir);
   }

   struct dirent* ent;
   while ((ent = readdir(dir)) != NULL) {
      if (ent->d_name[0] == '.')
         continue;
      struct package_info info;
      const char* name = ent->d_name;
      char* dirpath = xstrcatl(base_dir, "/", name, NULL);
      char* path = xstrcat(dirpath, suffix);

      struct stat st;
      check0(lstat(dirpath, &st));

      info.provided_name = xstrdup(name);
      info.is_provided = (st.st_mode & S_IFMT) == S_IFLNK;
      info.pkg = info.is_provided ? NULL : parse_package(path);      
      info.provider_name = info.is_provided ? xreadlink(dirpath) : info.provided_name;
      if (!info.provider_name)
         fail("find_packages(): Failed to readlink('%s')", dirpath);

      buf_push(*pkgs, info);
      free(dirpath);
      free(path);
   }

   for (size_t i = 0; i < buf_len(*pkgs); ++i) {
      struct package_info* info = &(*pkgs)[i];
      if (info->is_provided) {
         for (size_t j = 0; j < buf_len(pkgs); ++j) {
            struct package_info* info2 = &(*pkgs)[j];
            if (info2->pkg && !strcmp(info->provider_name, info2->pkg->name)) {
               info->pkg = info2->pkg;
               break;
            }
         }
      }
      if (!info->pkg) {
         warn("Failed to resolve package '%s'", info->provided_name);
      }
   }

   qsort(*pkgs, buf_len(*pkgs), sizeof(struct package_info), &pkg_info_cmp);

   return true;
}
void free_package_infos(struct package_info** infos) {
   for (size_t i = 0; i < buf_len(*infos); ++i) {
      struct package_info* info = &(*infos)[i];
      free(info->provided_name);
      if (info->is_provided) {
         free(info->provider_name);
      } else {
         free_package(info->pkg);
      }
   }
   buf_free(*infos);
}
static int search_func(const void* key, const void* info) {
   return strcmp((const char*)key, ((const struct package_info*)info)->pkg->name);
}
struct package_info* find_package_info(struct package_info* const* pkgs, const char* pkg) {
   return bsearch(pkg, *pkgs, buf_len(*pkgs), sizeof(struct package_info), search_func);
}
