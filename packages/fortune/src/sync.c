#include <stdio.h>
#include <stdlib.h>
#include "sync.h"
#include "packsync.h"
#include <dirent.h>
#include <string.h>

int cmd_search(const char *query) {
    DIR *dir = opendir(RECIPES_DIR);
    if (!dir) {
        printf("No hay índice local sincronizado. Ejecute 'fortune sync' primero.\n");
        return -1;
    }

    printf("Buscando paquetes que coincidan con '%s'...\n\n", query);
    int found = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        // Si el nombre del paquete contiene la query o no hay query, lo muestra
        if (!query || strstr(entry->d_name, query) != NULL) {
            printf("  -> %s\n", entry->d_name);
            found++;
        }
    }

    closedir(dir);

    if (found == 0) {
        printf("No se encontraron paquetes.\n");
    } else {
        printf("\nTotal encontrados: %d\n", found);
    }

    return 0;
}

int fortune_sync(void) {
    return fortune_sync_repository(RECIPES_REPO_URL, RECIPES_DIR);
}
