#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include "packsync.h"

#define RAW_REPO_URL "https://raw.githubusercontent.com/Nicolas-CNE/clover-packages/main"

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

static int download_file(const char *url, const char *dest_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    FILE *fp = fopen(dest_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? 0 : -1;
}

int fortune_sync_repository(const char *repo_url, const char *target_dir) {
    (void)repo_url;
    char cmd[512];
    char index_url[512];
    char dest_path[512];

    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", target_dir);
    system(cmd);

    snprintf(index_url, sizeof(index_url), "%s/PACKINDEX.txt", RAW_REPO_URL);
    snprintf(dest_path, sizeof(dest_path), "%s/PACKINDEX.txt", target_dir);

    printf("\033[34m[INFO]\033[0m Sincronizando índice desde %s...\n", index_url);

    if (download_file(index_url, dest_path) != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m Falló la descarga de PACKINDEX.txt\n");
        return -1;
    }

    printf("\033[32m[OK]\033[0m Índice sincronizado correctamente en %s\n", dest_path);
    return 0;
}

int fetch_recipe_if_missing(const char *pkg_name, const char *target_dir) {
    char recipe_dir[512];
    char recipe_file[512];
    char recipe_url[512];

    snprintf(recipe_dir, sizeof(recipe_dir), "%s/%s", target_dir, pkg_name);
    snprintf(recipe_file, sizeof(recipe_file), "%s/%s.recipe", recipe_dir, pkg_name);

    // Si la receta ya existe localmente, no descarga nada
    if (access(recipe_file, F_OK) == 0) {
        return 0;
    }

    printf("\033[34m[INFO]\033[0m Descargando receta para '%s'...\n", pkg_name);

    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", recipe_dir);
    system(mkdir_cmd);

    snprintf(recipe_url, sizeof(recipe_url), "%s/packages/%s/%s.recipe", RAW_REPO_URL, pkg_name, pkg_name);

    if (download_file(recipe_url, recipe_file) != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo descargar la receta desde %s\n", recipe_url);
        unlink(recipe_file);
        return -1;
    }

    return 0;
}
