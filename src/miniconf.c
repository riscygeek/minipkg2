#include <ctype.h>
#include "miniconf.h"
#include "utils.h"

static int isvalidname(int ch) {
   return isalnum(ch) || ch == '_';
}
static int isspacennl(int ch) {
   return isspace(ch) && ch != '\n';
}

miniconf_t* miniconf_parse(FILE* file, char** errmsg) {
   miniconf_t* conf = new(miniconf_t);

   size_t linenum = 1;
   int ch;
   char* cursection = NULL;
   while ((ch = fgetc(file)) != EOF) {
      if (isspace(ch)) {
         if (ch == '\n')
            ++linenum;
         continue;
      }
      if (ch == '#') {
         while ((ch = fgetc(file)) != EOF && ch != '\n');
         ++linenum;
         continue;
      }

      if (ch == '[') {
         buf_clear(cursection);
         while ((ch = fgetc(file)) != ']') {
            if (isvalidname(ch)) {
               buf_push(cursection, ch);
            } else {
               if (ch == EOF) {
                  *errmsg = xstrdup("Unexpected end of file.");
               } else if (ch == '\n') {
                  *errmsg = xstrdup("Unexpected line break.");
               } else {
                  const char chbuf[2] = { ch, '\0' };
                  *errmsg = xstrcatl("Invalid section name character '", chbuf, "'.");
               }
               goto failed;
            }
         }
         if ((ch = fgetc(file)) != '\n') {
            const char chbuf[2] = { ch, '\0' };
            *errmsg = xstrcatl("Expected line break, got '", chbuf, "'.");
            goto failed;
         }
      } else {
         if (!isvalidname(ch)) {
            const char chbuf[2] = { ch, '\0' };
            *errmsg = xstrcatl("Invalid input: '", chbuf, "'.");
            goto failed;
         }
         char* namebuf = NULL;
         buf_resize(namebuf, buf_len(cursection));
         memcpy(namebuf, cursection, buf_len(namebuf));
         buf_push(namebuf, '.');

         while (isvalidname(ch)) {
            buf_push(namebuf, ch);
            ch = fgetc(file);
         }

         while (isspacennl(ch))
            ch = fgetc(file);

         if (ch != '=') {
            const char chbuf[2] = { ch, '\0' };
            *errmsg = xstrcatl("Expected '=', got '", chbuf, "'.");
            buf_free(namebuf);
            goto failed;
         }
         buf_push(namebuf, '\0');
         char* name = xstrdup(namebuf);
         buf_free(namebuf);

         // Skip spaces after the '='.
         while (isspacennl(ch = fgetc(file)));

         // Read the value.
         char* valuebuf = NULL;
         while (ch != '\n') {
            buf_push(valuebuf, ch);
            ch = fgetc(file);
         }
         ++linenum;

         // Trim spaces at the end.
         while (buf_len(valuebuf) > 0 && isspace(buf_last(valuebuf)))
            buf_remove(valuebuf, buf_size(valuebuf)-1, 1);

         buf_push(valuebuf, '\0');
         char* value = xstrdup(valuebuf);
         buf_free(valuebuf);
         
         struct miniconf_setting setting = { name, value };
         buf_push(conf->settings, setting);
      }
   }

   return conf;
failed:
   buf_free(cursection);
   miniconf_free(conf);
   char linenum_s[20];
   snprintf(linenum_s, sizeof(linenum_s), "%zu: ", linenum);
   char* tmp = *errmsg;
   *errmsg = xstrcat(linenum_s, tmp);
   free(tmp);
   return NULL;
}
const char* miniconf_get(const miniconf_t* conf, const char* name) {
   for (size_t i = 0; i < buf_len(conf->settings); ++i) {
      if (!strcmp(name, conf->settings[i].name))
         return conf->settings[i].value;
   }
   return NULL;
}
void miniconf_dump(const miniconf_t* conf, FILE* file) {
   for (size_t i = 0; i < buf_len(conf->settings); ++i) {
      const struct miniconf_setting* s = &conf->settings[i];
      fprintf(file, "config[%s]='%s'\n", s->name, s->value);
   }
}
void miniconf_free(miniconf_t* conf) {
   for (size_t i = 0; i < buf_len(conf->settings); ++i) {
      free(conf->settings[i].name);
      free(conf->settings[i].value);
   }
   buf_free(conf->settings);
   free(conf);
}
