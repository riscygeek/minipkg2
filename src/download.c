#include <unistd.h>
#include "minipkg2.h"
#include "utils.h"
#include "print.h"

#if HAS_LIBCURL
#include <curl/curl.h>

static CURL* curl = NULL;

static void cleanup_curl(void) {
   curl_easy_cleanup(curl);
   curl = NULL;
}
#endif

bool download(const char* url, const char* dest, bool overwrite) {
   if (!overwrite && access(dest, F_OK) == 0)
      return true;
#if HAS_LIBCURL
   if (!curl) {
      curl = curl_easy_init();
      if (unlikely(!curl)) {
         error("Failed to initialize libcurl.");
         return false;
      }
      atexit(cleanup_curl);
   }

   if (!mkparentdirs(dest, 0755)) {
      error_errno("Failed to create parent directory of '%s'", dest);
      return false;
   }

   FILE* file = fopen(dest, "wb");
   if (!file) {
      error_errno("Failed to open file '%s'", dest);
      return false;
   }

   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

   CURLcode result = curl_easy_perform(curl);
   if (result != CURLE_OK) {
      error("Failed to download '%s': %s", url, curl_easy_strerror(result));
   }

   fclose(file);
   return result == CURLE_OK;
#else
   (void)url;
   (void)dest;
   error("Downloading files is not supported because minipkg2 was not built with curl support. Please rebuild minipkg2.");
   return false;
#endif
}
