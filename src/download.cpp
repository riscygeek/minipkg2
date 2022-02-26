#include <unistd.h>
#include "utils.hpp"
#include "print.hpp"

#if HAS_LIBCURL
# include <curl/curl.h>
#endif

namespace minipkg2 {
    bool download(const std::string& url, const std::string& dest, bool overwrite) {
        if (!overwrite && ::access(dest.c_str(), F_OK) == 0)
            return true;

#if HAS_LIBCURL
        static CURL* curl = nullptr;

        if (!curl) {
            curl = ::curl_easy_init();
            if (!curl) {
                printerr(color::ERROR, "Failed to initialize libcurl.");
                return false;
            }
            ::atexit([]{
                curl_easy_cleanup(curl);
                curl = nullptr;
            });
        }


        if (!mkparentdirs(dest, 0755)) {
            printerr(color::ERROR, "Failed to create parent directories of {}.", dest);
            return false;
        }

        std::FILE* file = std::fopen(dest.c_str(), "wb");
        if (!file) {
            printerr(color::ERROR, "Failed to open file '{}'", dest);
            return false;
        }

        ::curl_easy_setopt(curl, CURLOPT_URL,             url.c_str());
        ::curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   fwrite);
        ::curl_easy_setopt(curl, CURLOPT_WRITEDATA,       file);
        ::curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,  1);

        const ::CURLcode result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            printerr(color::ERROR, "Failed to download '{}': {}", url, ::curl_easy_strerror(result));
        }

        std::fclose(file);

        return result == CURLE_OK;
#else
        (void)url;
        printerr(color::ERROR, "Downloading files is not supported because minipkg2 was not built with curl support. Please rebuild minipkg2.");
        return false;
#endif
    }
}
