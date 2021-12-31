#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "print.h"

static void vprint(int color, const char* fmt, va_list ap) {
   fprintf(stderr, "\033[%dm*\033[0m ", color);
   vfprintf(stderr, fmt, ap);
}

void print(int color, const char* fmt, ...) {
   va_list ap;
   va_start(ap, fmt);

   vprint(color, fmt, ap);
   fputc('\n', stderr);

   va_end(ap);
}
bool yesno(const char* q, bool def, ...) {
   va_list ap;
   va_start(ap, def);

   vprint(34, q, ap);
   fputs(def ? " [Y/n] " : " [y/N] ", stderr);

   va_end(ap);

   char buffer[32];
   fgets(buffer, sizeof(buffer), stdin);

   return tolower(buffer[0]) == 'y';
}
void print_errno(int color, const char* fmt, ...) {
   const int saved_errno = errno;
   va_list ap;
   va_start(ap, fmt);

   vprint(color, fmt, ap);
   fprintf(stderr, ": %s\n", strerror(saved_errno));
   va_end(ap);
}
