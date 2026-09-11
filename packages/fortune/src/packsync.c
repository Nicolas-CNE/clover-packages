#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include "packsync.h"

// Callback para escribir los datos descargados en el archivo temporal
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

int fortune_sync_repository(const char *repo_url, const char *target_dir) {
    const char *tmp_archive = "/tmp/fortune_recipes.tar.gz";
    char cmd[2048];

    printf("\033[34m[INFO]\033[0m Sincronizando recetas desde %s...\n", repo_url);

    // 1. Descargar el archivo con libcurl
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo inicializar libcurl.\n");
        return -1;
    }

    FILE *fp = fopen(tmp_archive, "wb");
    if (!fp) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo crear el archivo temporal %s\n", tmp_archive);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, repo_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m Error al descargar recetas: %s\n", curl_easy_strerror(res));
        unlink(tmp_archive);
        return -1;
    }

    // 2. Asegurar que el directorio de destino exista
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", target_dir);
    system(cmd);

    // 3. Extraer ÚNICAMENTE las recetas (descartando READMEs, scripts y licencias)
    // --strip-components=2 remueve el directorio raiz de GitHub (ej: clover-packages-main/) y la carpeta packages/
   // las wildcards no funcionaron :d
    snprintf(cmd, sizeof(cmd),
         "tar -xf \"%s\" -C \"%s\" --strip-components=2 \"*/packages/*\"",
         tmp_archive, target_dir);
    
    int status = system(cmd);
    unlink(tmp_archive);

    if (status != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m Falló la extracción selectiva de las recetas.\n");
        return -1;
    }

    printf("\033[32m[OK]\033[0m Recetas sincronizadas limpiamente en %s\n", target_dir);
    return 0;
}
