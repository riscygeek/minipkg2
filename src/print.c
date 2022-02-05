#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include "minipkg2.h"
#include "print.h"
#include "utils.h"

enum verbosity_level verbosity = V_NORMAL;

static FILE* logfile = NULL;

static void fini_log(void) {
   if (logfile != NULL)
      fclose(logfile);
}

void init_log(void) {
   static bool first = true;
   if (first) {
      first = false;
      atexit(fini_log);
   } else fini_log();

   setlocale(LC_TIME, "C.UTF-8");

   char* logdir = xstrcat(root, "/log");
   char* path = xstrcat(logdir, "/minipkg2.log");

   mkdir_p(logdir, 0755);
   free(logdir);

   logfile = fopen(path, "a");
   free(path);
}

static void vlprint(const char* fmt, va_list ap) {
   if (!logfile)
      return;

   const time_t timer = time(NULL);
   struct tm* tm = localtime(&timer);
   char buffer[100];

   if (strftime(buffer, sizeof buffer, "%A %c", tm)) {
      fprintf(logfile, "[%s] ", buffer);
   }
   vfprintf(logfile, fmt, ap);
}

void lprint(const char* msg, ...) {
   va_list ap;
   va_start(ap, msg);

   vlprint(msg, ap);

   va_end(ap);
}
void lprintln(const char* msg, ...) {
   va_list ap;
   va_start(ap, msg);

   vlprint(msg, ap);

   if (logfile)
      fputc('\n', logfile);

   va_end(ap);
}

static void vprint(int color, const char* fmt, va_list ap) {
   if (color)
      fprintf(stderr, "\033[%dm*\033[0m ", color);
   vfprintf(stderr, fmt, ap);
}

void print(int color, const char* fmt, ...) {
   va_list ap;
   va_start(ap, fmt);

   vprint(color, fmt, ap);

   va_end(ap);
}
void println(int color, const char* fmt, ...) {
   va_list ap;
   va_start(ap, fmt);

   vprint(color, fmt, ap);
   fputc('\n', stderr);

   va_end(ap);
}
bool yesno(const char* q, bool def, ...) {
   va_list ap;
   va_start(ap, def);

   vprint(COLOR_INFO, q, ap);
   fputs(def ? " [Y/n] " : " [y/N] ", stderr);

   va_end(ap);

   char buffer[32];
   fgets(buffer, sizeof(buffer), stdin);

   return buffer[0] != '\n' ? tolower(buffer[0]) == 'y' : def;
}
void print_errno(int color, const char* fmt, ...) {
   const int saved_errno = errno;
   va_list ap;
   va_start(ap, fmt);

   vprint(color, fmt, ap);
   fprintf(stderr, ": %s\n", strerror(saved_errno));
   va_end(ap);
}
