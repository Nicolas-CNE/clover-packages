#include <stdio.h>
#include <stdlib.h>
#include "sync.h"

int fortune_sync(void) {
    printf("[INFO] Sincronizando repositorio de recetas...\n");

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "mkdir -p /tmp/fortune_sync && "
        "curl -sL \"%s\" -o /tmp/fortune_sync/recipes.tar.gz && "
        "mkdir -p %s && "
        "tar -xf /tmp/fortune_sync/recipes.tar.gz -C %s --strip-components=1 && "
        "rm -rf /tmp/fortune_sync",
        RECIPES_REPO_URL, RECIPES_DIR, RECIPES_DIR);

    int status = system(cmd);
    if (status == 0) {
        printf("[INFO] Base de recetas actualizada correctamente en %s.\n", RECIPES_DIR);
        return 0;
    } else {
        fprintf(stderr, "[ERROR] Falló la sincronización de recetas.\n");
        return 1;
    }
}
