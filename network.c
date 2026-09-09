#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MemoryBuffer {
    char *data;
    size_t size;
};

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    struct MemoryBuffer *mem = (struct MemoryBuffer *)userdata;

    size_t new_size = mem->size + realsize;
    if (new_size < mem->size) return 0;

    char *ptr_new = realloc(mem->data, new_size + 1);
    if (!ptr_new) return 0;

    mem->data = ptr_new;
    memcpy(&(mem->data[mem->size]), ptr, realsize);
    mem->size = new_size;
    mem->data[mem->size] = 0;

    return realsize;
}

static size_t file_write_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static int progress_callback(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)p;
    (void)ultotal;
    (void)ulnow;
    if (dltotal <= 0) return 0;
    int progress = (int)((dlnow * 100) / dltotal);

    printf("\r\033[32m[DOWNLOAD]\033[0m Progress: [");
    for (int i = 0; i < 50; i++) {
        if (i < progress / 2) printf("=");
        else if (i == progress / 2) printf(">");
        else printf(" ");
    }
    printf("] %d%%", progress);
    fflush(stdout);
    return 0;
}

int net_download_file(const char *url, const char *output_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    FILE *fp = fopen(output_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FortunePM/2.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    printf("\n");

    fclose(fp);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK) ? 0 : -1;
}

char *net_fetch_string(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct MemoryBuffer chunk = { .data = malloc(1), .size = 0 };
    if (!chunk.data) {
        curl_easy_cleanup(curl);
        return NULL;
    }
    chunk.data[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FortunePM/2.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(chunk.data);
        return NULL;
    }

    return chunk.data;
}
