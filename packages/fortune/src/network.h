#ifndef FORTUNE_NETWORK_H
#define FORTUNE_NETWORK_H

#include <curl/curl.h>

int net_download_file(const char *url, const char *output_path);
char *net_fetch_string(const char *url);

#endif
