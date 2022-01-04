#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include "utils.h"
#include "print.h"
#include "buf.h"

void* xmalloc(size_t sz) {
   void* ptr = calloc(1, sz);
   if (unlikely(!ptr))
      fail("Failed to allocate %zu bytes.");
   return ptr;
}
char* xstrdup(const char* str) {
   char* new_str = strdup(str);
   if (unlikely(!new_str))
      fail("Failed to strcopy(\"%zu\").", strlen(str));
   return new_str;
}
char* xstrcat(const char* s1, const char* s2) {
   const size_t l1 = strlen(s1);
   const size_t l2 = strlen(s2);
   char* new_str = xmalloc(l1 + l2 + 1);
   memcpy(new_str, s1, l1);
   memcpy(new_str + l1, s2, l2);
   new_str[l1 + l2] = '\0';
   return new_str;
}
char* xstrcatl_impl(const char* s1, const char* s2, ...) {
   va_list ap;
   va_start(ap, s2);
   char* new_str = xstrcat(s1, s2);
   const char* sx;
   while ((sx = va_arg(ap, const char*)) != NULL) {
      char* old_str = new_str;
      new_str = xstrcat(old_str, sx);
      free(old_str);
   }
   return new_str;
}
char* freadline(FILE* file) {
   char* buf = NULL;
   int ch;
   if (feof(file))
      return NULL;
   while ((ch = fgetc(file)) != EOF) {
      if (ch == '\n')
         break;

      if (ch == '\\') {
         ch = fgetc(file);
         if (ch == '\n')
            continue;
         buf_push(buf, '\\');
      }
      buf_push(buf, ch);
   }


   buf_push(buf, '\0');
   char* str = xstrdup(buf);
   buf_free(buf);
   return str;
}
char** freadlines(FILE* file) {
   char** lines = NULL;
   char* line;
   while ((line = freadline(file)) != NULL)
      buf_push(lines, line);
   return lines;
}

bool ends_with(const char* s1, const char* s2) {
   const size_t l1 = strlen(s1);
   const size_t l2 = strlen(s2);
   return l1 >= l2 && !memcmp(s1 + (l1 - l2), s2, l2);
}
char* xreadlink(const char* path) {
   char* buf = xmalloc(PATH_MAX + 1);
   if (readlink(path, buf, PATH_MAX) < 0)
      return free(buf), NULL;
   return buf;
}

char* fread_file(FILE* file) {
   char* buf = NULL;
   int ch;
   while ((ch = fgetc(file)) != EOF)
      buf_push(buf, ch);
   buf_push(buf, '\0');
   char* str = xstrdup(buf);
   buf_free(buf);
   return str;
}
char* read_file(const char* path) {
   FILE* file = fopen(path, "r");
   if (!file)
      return NULL;
   char* str = fread_file(file);
   fclose(file);
   return str;
}

bool mkparentdirs(const char* dir, mode_t mode) {
   char* buffer = xstrdup(dir);

   char* end = buffer + 1;
   while ((end = strchr(end, '/')) != NULL) {
      *end = '\0';
      const int ec = mkdir(buffer, mode);
      if (ec != 0 && errno != EEXIST)
         return free(buffer), false;
      *end = '/';
      ++end;
   }
   free(buffer);
   return true;
}
bool mkdir_p(const char* dir, mode_t mode) {
   if (!mkparentdirs(dir, mode))
      return false;
   const int ec = mkdir(dir, mode);
   return ec != 0 && ec != EEXIST;
}
bool rm_rf(const char* path) {
   struct stat st;
   if (lstat(path, &st) != 0)
      return true;

   bool success = true;
   if ((st.st_mode & S_IFMT) == S_IFDIR) {
      DIR* dir = opendir(path);
      if (!dir)
         return false;

      struct dirent* ent;
      while ((ent = readdir(dir)) != NULL) {
         // Skip '.' and '..'
         if (xstreql(ent->d_name, ".", ".."))
            continue;

         char* new_path = xstrcatl(path, "/", ent->d_name);
         success &= rm_rf(new_path);
         free(new_path);
      }

      closedir(dir);
   }
   if (verbosity >= 3) {
      log("Deleting %s...", path);
   }
   success &= remove(path) == 0;
   return success;
}
bool copy_file(const char* source, const char* dest) {
   const int input = open(source, O_RDONLY);
   if (input < 0)
      return false;

   const int output = creat(dest, 0644);
   if (output < 0) {
      close(input);
      return false;
   }

   struct stat st;
   if (fstat(input, &st) != 0) {
      close(input);
      close(output);
      return false;
   }

   const ssize_t res = sendfile(output, input, NULL, st.st_size);

   close(input);
   close(output);

   return res >= 0;
}
bool create_archive(const char* file, const char* path) {
   DIR* dir = opendir(path);
   if (!dir)
      return false;

   pid_t pid;
   check(pid = vfork(), >= 0);

   if (pid == 0) {
      char** args = NULL;

      buf_push(args, "tar");
      buf_push(args, "-czf");
      buf_push(args, xstrdup(file));
      buf_push(args, "--");

      struct dirent* ent;
      while ((ent = readdir(dir)) != NULL) {
         if (xstreql(ent->d_name, ".", ".."))
            continue;
         buf_push(args, xstrdup(ent->d_name));
      }
      closedir(dir);

      buf_push(args, NULL);
      chdir(path);
      execvp("tar", args);
      _exit(1);
   } else {
      return waitexit(pid, 10) == 0;
   }
}
int waitexit(pid_t pid, int limit) {
   int cnt = 0;
   int wstatus;
   do {
      waitpid(pid, &wstatus, 0);
      if (++cnt == limit)
         fail_errno("Failed to wait for sub-process");
   } while (!WIFEXITED(wstatus));
   return WEXITSTATUS(wstatus);
}
bool write_file(const char* filename, const char* data) {
   FILE* file = fopen(filename, "w");
   if (!file)
      return false;
   fputs(data, file);
   fclose(file);
   return true;
}
bool print_file(FILE* to, const char* filename) {
   FILE* file = fopen(filename, "r");
   if (!file)
      return false;
   char buffer[100];
   while (fgets(buffer, sizeof(buffer)-1, file) != NULL)
      fputs(buffer, to);
   fclose(file);
   return true;
}
void redir_file(FILE* from, FILE* to) {
   char buf[100];
   while (fgets(buf, sizeof(buf)-1, from) != NULL)
      fputs(buf, to);
}
void format_size(size_t* sz, const char** unit_out) {
   static const struct unit {
      const char* name;
      size_t base;
   } units[] = {
#if SIZE_MAX >= (1ull << 40)
      {"TiB", (size_t)1 << 40},
#endif
      {"GiB", 1 << 30},
      {"MiB", 1 << 20},
      {"KiB", 1 << 10},
      {"B",         0},
      {NULL},
   };
   for (size_t i = 0; units[i].name; ++i) {
      if (*sz >= units[i].base) {
         *sz = *sz / units[i].base;
         *unit_out = units[i].name;
         return;
      }
   }
   fail("format_size(): unreachable reached.");
}
bool is_symlink(const char* path) {
   struct stat st;
   return lstat(path, &st) == 0 && ((st.st_mode & S_IFMT) == S_IFLNK);
}
bool dir_is_empty(const char* path) {
   DIR* dir;
   check(dir = opendir(path), != NULL);

   struct dirent* ent;
   bool empty = true;
   while ((ent = readdir(dir)) != NULL) {
      if (xstreql(ent->d_name, ".", ".."))
         continue;
      empty = false;
      break;
   }
   closedir(dir);
   return empty;
}
bool xstreql_impl(const char* s1, ...) {
   va_list ap;
   va_start(ap, s1);

   const char* sx;
   bool found = false;
   while ((sx = va_arg(ap, const char*)) != NULL) {
      if (!strcmp(s1, sx)) {
         found = true;
         break;
      }
   }
   va_end(ap);
   return found;
}
bool is_dir(const char* path) {
   struct stat st;
   return stat(path, &st) == 0 && ((st.st_mode & S_IFMT) == S_IFDIR);
}
char* xpread(const char* cmd) {
   FILE* file = popen(cmd, "r");
   if (!file)
      return NULL;

   char* reply = fread_file(file);

   const int ec = pclose(file);
   if (ec != 0)
      return free(reply), NULL;

   const size_t len = strlen(reply);
   if (len >= 1 && reply[len-1] == '\n')
      reply[len-1] = '\0';

   return reply;
}
void free_strlist(char*** list) {
   for (size_t i = 0; i < buf_len(*list); ++i) {
      free((*list)[i]);
   }
   buf_free(*list);
}
void strlist_remove(char*** list, const char* str) {
   for (size_t i = 0; i < buf_len(*list); ) {
      if (!strcmp((*list)[i], str)) {
         buf_remove(*list, i, 1);
      } else {
         ++i;
      }
   }
}
