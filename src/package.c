#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
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

      // Run the shell (by default bash).
      execlp(SHELL, SHELL, NULL);
      error_errno("Failed to run bash");
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
