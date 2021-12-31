#include <string.h>
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
char* freadline(FILE* file) {
   char* buf = NULL;
   char ch;
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

bool ends_with(const char* s1, const char* s2) {
   const size_t l1 = strlen(s1);
   const size_t l2 = strlen(s2);
   return l1 >= l2 && !memcmp(s1 + (l1 - l2), s2, l2);
}
