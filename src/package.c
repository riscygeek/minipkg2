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

#define SHELL_SCRIPT_HEADER                                                                     \
   "[[ -f $ROOT/usr/lib/minipkg2/env.bash ]] && source \"$ROOT/usr/lib/minipkg2/env.bash\"\n"   \
   "source \"$pkgfile\"\n"

static bool pkg_path_is_valid(const char* path) {
   return path[0] != '.' && is_dir(path);
}

static bool is_empty(const char* s) {
   return !s || !s[0];
}

static void remap(int fd, int newfd) {
   check0f(close(fd));
   checkvf(dup(newfd), fd);
}


static char** read_list(FILE* file) {
   char** list = NULL;
   char* line;

   while (!is_empty(line = freadline(file)))
      buf_push(list, line);

   // This deletes only the last (empty) line.
   free(line);

   return list;
}

struct package* parse_package(const char* path) {
   // Check if `path` can be read.
   if (access(path, R_OK) != 0) {
      error_errno("Cannot access '%s'", path);
      return NULL;
   }

   struct package* pkg = new(struct package);
   pkg->pkgfile = xstrdup(path);
   
   // The shell script that will be run.
   static const char shell_script[] =
      SHELL_SCRIPT_HEADER
      "echo \"$pkgname\"\n"
      "echo \"$pkgver\"\n"
      "echo \"$url\"\n"
      "echo \"$description\"\n"
      "for src in \"${sources[@]}\"; do\n"
      "  echo \"$src\"\n"
      "done\n"
      "echo\n"
      "for pkg in \"${depends[@]}\"; do\n"
      "  echo \"$pkg\"\n"
      "done\n"
      "echo\n"
      "for pkg in \"${provides[@]}\"; do\n"
      "  echo \"$pkg\"\n"
      "done\n"
      "echo\n"
      "for pkg in \"${conflicts[@]}\"; do\n"
      "  echo \"$pkg\"\n"
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

      // Remap stdin, stdout, stderr.
      remap(STDIN_FILENO, pipefd[0][0]);
      remap(STDOUT_FILENO, pipefd[1][1]);
      remap(STDERR_FILENO, open("/dev/null", O_WRONLY));

      // Run the shell (by default bash).
      execlp(SHELL, SHELL, NULL);
      _exit(1);
   } else {
      // Close unused pipes
      close(pipefd[0][0]);
      close(pipefd[0][1]);
      close(pipefd[1][1]);

      // Wait for the program to exit.
      int ec = waitexit(pid, 10);
      if (ec != 0)
         fail("Sub-process exited with %d", ec);
      
      FILE* file;
      check(file = fdopen(pipefd[1][0], "r"), != NULL);

      // Read package parameters.
      pkg->name         = freadline(file);
      pkg->version      = freadline(file);
      pkg->url          = freadline(file);
      pkg->description  = freadline(file);
      pkg->sources   = read_list(file);
      pkg->depends   = read_list(file);
      pkg->provides  = read_list(file);
      pkg->conflicts = read_list(file);

      fclose(file);
   }

   return pkg;
}
void free_package(struct package* pkg) {
   free(pkg->pkgfile);
   free(pkg->name);
   free(pkg->version);
   free(pkg->url);
   free(pkg->description);
   free_strlist(&pkg->sources);
   free_strlist(&pkg->depends);
   free_strlist(&pkg->provides);
   free_strlist(&pkg->conflicts);
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
   print_line("Provides",     buf_len(pkg->provides), pkg->provides);
   print_line("Conflicts",    buf_len(pkg->conflicts), pkg->conflicts);
}
struct package* find_package(const char* name, enum package_source src) {
   char* path;
   if (!pkg_path_is_valid(name))
      return NULL;
   switch (src) {
   case PKG_LOCAL:
      path = xstrcatl(pkgdir, "/", name, "/package.info");
      break;
   case PKG_REPO:
      path = xstrcatl(repodir, "/", name, "/package.build");
      break;
   default:
      fail("find_package(): Invalid package source %d", src);
   }
   struct package* pkg = (access(path, R_OK) == 0) ? parse_package(path) : NULL;
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
      char* dirpath = xstrcatl(base_dir, "/", name);
      if (!pkg_path_is_valid(dirpath)) {
         free(dirpath);
         continue;
      }
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
bool pkg_is_installed(const char* name) {
   char* dir = xstrcatl(pkgdir, "/", name);
   struct stat st;
   const bool exists = stat(dir, &st) == 0;
   free(dir);
   return exists;
}
bool pkg_build(struct package* pkg, const char* bmpkg, const char* filesdir) {
   char* pkg_basedir    = xstrcatl(builddir, "/", pkg->name, "-", pkg->version);
   char* pkg_srcdir     = xstrcat(pkg_basedir, "/src");
   char* pkg_builddir   = xstrcat(pkg_basedir, "/build");
   char* pkg_pkgdir     = xstrcat(pkg_basedir, "/pkg");
   char* path_logfile   = xstrcat(pkg_basedir, "/log");

   mkdir_p(pkg_builddir, 0755);
   checkv(rm_rf(pkg_pkgdir), true);

   
   static const char shell_script[] = {
      SHELL_SCRIPT_HEADER
      "cd \"$builddir\"\n"
      "S=\"$srcdir\"\n"
      "B=\"$builddir\"\n"
      "F=\"$filesdir\"\n"
      "echo \"prepare()\"\n"
      "prepare || exit 1\n"
      "echo \"build()\"\n"
      "build || exit 1\n"
      "D=\"$pkgdir\"\n"
      "echo \"package()\"\n"
      "package || exit 1\n"
      "exit 0\n"
   };

   // pipefd[0] : minipkg2 -> bash
   // pipefd[1] : minipkg2 <- bash (stdout & stderr)
   int pipefd[2][2];

   check0(pipe(pipefd[0]));
   check0(pipe(pipefd[1]));

   check(write(pipefd[0][1], shell_script, sizeof(shell_script) - 1), == (sizeof(shell_script) - 1));

   // TODO: use posix_spawn() instead

   pid_t pid;
   check(pid = vfork(), >= 0);

   if (pid == 0) {
      // Set environment variables.
      setenv("ROOT",       root,          1);
      setenv("pkgfile",    pkg->pkgfile,  1);
      setenv("srcdir",     pkg_srcdir,    1);
      setenv("builddir",   pkg_builddir,  1);
      setenv("pkgdir",     pkg_pkgdir,    1);
      setenv("filesdir",   filesdir,      1);

      // Close unused pipes.
      close(pipefd[0][1]);
      close(pipefd[1][0]);

      // Remap stdin, stdout, stderr.
      remap(STDIN_FILENO, pipefd[0][0]);
      remap(STDOUT_FILENO, pipefd[1][1]);
      remap(STDERR_FILENO, pipefd[1][1]);

      // Run the shell.
      execlp(SHELL, SHELL, NULL);
      _exit(1);
   } else {
      // Close unused pipes.
      close(pipefd[0][0]);
      close(pipefd[0][1]);
      close(pipefd[1][1]);

      FILE* logfile;
      check(logfile = fopen(path_logfile, "w"), != NULL);

      FILE* log;
      check(log = fdopen(pipefd[1][0], "r"), != NULL);
      
      char buffer[100];
      while (fgets(buffer, sizeof(buffer) - 1, log) != NULL) {
         if (verbosity >= V_VERBOSE)
            fputs(buffer, stderr);
         fputs(buffer, logfile);
      }
      fclose(logfile);
      fclose(log);


      int ec = waitexit(pid, 10);

      if (ec != 0)
         print_file(stderr, path_logfile);

      free(pkg_basedir);
      free(pkg_srcdir);
      free(pkg_builddir);

      if (ec != 0) {
         error("Failed to build %s. Log file: '%s'", pkg->name, path_logfile);
         free(pkg_pkgdir);
         free(path_logfile);
         return false;
      }
      free(path_logfile);
   }

   // Create a binary package.
   
   // Copy the metadata.
   char* metadir        = xstrcat(pkg_pkgdir, "/.meta");
   char* meta_name      = xstrcat(metadir, "/name");
   char* meta_version   = xstrcat(metadir, "/version");
   char* meta_pkginfo   = xstrcat(metadir, "/package.info");
   if (!is_dir(metadir))
      check0(mkdir(metadir, 0755));
   checkv(write_file(meta_name, pkg->name), true);
   checkv(write_file(meta_version, pkg->version), true);
   checkv(copy_file(pkg->pkgfile, meta_pkginfo), true);
   free(meta_name);
   free(meta_version);
   free(meta_pkginfo);
   free(metadir);

   // Create archive
   create_archive(bmpkg, pkg_pkgdir);

   free(pkg_pkgdir);
   return true;
}
bool pkg_download_sources(struct package* pkg) {
   bool success = true;
   for (size_t i = 0; i < buf_len(pkg->sources); ++i) {
      const char* url = pkg->sources[i];
      const char* filename = strrchr(url, '/');
      if (!filename)
         fail("Invalid URL: %s", url);
      ++filename;
      char* path = xstrcatl(builddir, "/", pkg->name, "-", pkg->version, "/src/", filename);

      if (!download(url, path, false)) {
         //error("Failed to download '%s'", url);
         success = false;
      }

      free(path);
   }
   return success;
}
bool binpkg_install(const char* binpkg) {
   // Read package name.
   char* cmd = xstrcatl("tar -xf '", binpkg, "' .meta/name -O");
   FILE* file;
   check(file = popen(cmd, "r"), != NULL);
   char* pkgname = fread_file(file);
   if (pclose(file) != 0) {
      error("Failed to read package name.");
      return false;
   }
   free(cmd);

   FILE* files;
   char* pkg_pkgdir = xstrcatl(pkgdir, "/", pkgname);

   // If there is a package install with the same name
   // find all files contained in it.
   char* path_files = xstrcat(pkg_pkgdir, "/files");
   files = fopen(path_files, "r");
   char** old_files = NULL;
   if (files) {
      old_files = freadlines(files);
      fclose(files);
   }

   mkdir_p(root, 0755);
   mkdir_p(pkg_pkgdir, 0755);

   // Create `$pkg_pkgdir/files` file
   // that contains a list of files that belog to this package.
   check(files = fopen(path_files, "w"), != NULL);
   cmd = xstrcatl("tar -tf '", binpkg, "' --exclude='.meta' | awk '{printf \"/%s\\n\", $0}'");
   check(file = popen(cmd, "r"), != NULL);
   redir_file(file, files);
   fclose(files);
   free(cmd);

   if (pclose(file) != 0) {
      free_strlist(&old_files);
      error("Failed to list package files.");
      return false;
   }

   // Copy `.meta/package.build` to `$pkg_pkgdir/package.info`
   char* path = xstrcat(pkg_pkgdir, "/package.info");
   FILE* pinfo;
   check(pinfo = fopen(path, "w"), != NULL);
   free(path);
   cmd = xstrcatl("tar -xf '", binpkg, "' .meta/package.info -O");
   check(file = popen(cmd, "r"), != NULL);
   free(cmd);
   redir_file(file, pinfo);
   fclose(pinfo);

   if (pclose(file) != 0) {
      error("Failed to create package.info");
      return false;
   }

   // Actually install the package by extracting it's contents into $root.
   cmd = xstrcatl("tar -C '", root, "' -xf '", binpkg, "' --exclude='.meta'");
   const int ec = system(cmd);
   free(cmd);

   if (ec != 0) {
      error("Failed to install package");
      return false;
   }


   // Find files that are part of the old package
   // but not in the new package...
   check(files = fopen(path_files, "r"), != NULL);
   char** new_files = freadlines(files);
   for (size_t i = 0; i < buf_len(new_files); ++i) {
      strlist_remove(&old_files, new_files[i]);
   }

   // ... and delete them.
   for (size_t i = 0; i < buf_len(old_files); ++i) {
      rm(old_files[i]);
   }


   free_strlist(&old_files);
   free_strlist(&new_files);

   // TODO: copy the binpkg to /var/cache/minipkg2/binpkgs/
   free(path_files);
   return true;
}
bool pkg_estimate_size(const char* name, size_t* size_out) {
   // Check if it is a symlink, if yes report a size of 0.
   char* dir = xstrcatl(pkgdir, "/", name);
   if (is_symlink(dir)) {
      free(dir);
      *size_out = 0;
      return true;
   }

   // Otherwise read the files file and add the filesize of each installed file.
   char* path_files = xstrcat(dir, "/files");
   free(dir);
   FILE* files = fopen(path_files, "r");
   if (!files) {
      error_errno("Failed to open '%s'", path_files);
      free(path_files);
      return false;
   }
   free(path_files);

   size_t sz = 0;
   char* line;
   while ((line = freadline(files)) != NULL) {
      struct stat st;
      if (lstat(line, &st) != 0)
         sz += st.st_size;
      free(line);
   }
   fclose(files);
   *size_out = sz;
   return true;
}
bool purge_package(const char* name) {
   char* dir = xstrcatl(pkgdir, "/", name);
   if (is_symlink(dir)) {
      const int ec = unlink(dir);
      if (ec != 0)
         error_errno("Failed to unlink '%s'", dir);
      free(dir);
      return ec != 0;
   }
   char* path_files = xstrcat(dir, "/files");
   FILE* files_file = fopen(path_files, "r");
   if (!files_file) {
      error_errno("Failed to open '%s'", path_files);
      free(path_files);
      return false;
   }
   char** files = NULL;
   char* line;
   while ((line = freadline(files_file)) != NULL) {
      if (line[0] != '\0') {
         char* path = xstrcatl(root, "/", line);
         buf_push(files, path);
      }
      free(line);
   }
   fclose(files_file);

   size_t num_removed;
   do {
      num_removed = 0;
   
      for (size_t i = 0; i < buf_len(files); ++i) {
         if (rm(files[i]) == 0) {
            free(files[i]);
            buf_remove(files, i, 1);
            ++num_removed;
         }
      }
   } while (num_removed != 0 && buf_len(files) != 0);
   bool success = true;
   for (size_t i = 0; i < buf_len(files); ++i) {
      struct stat st;
      // Check if file even exists.
      if (stat(files[i], &st) != 0)
         continue;
      
      if ((st.st_mode & S_IFMT) == S_IFDIR && !is_empty(files[i]))
         continue;

      error("Failed to delete file '%s'", files[i]);
      success = false;
   }
   if (!success)
      return free(dir), false;

   rm_rf(dir);
   return true;
}
