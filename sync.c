#include <stdio.h>
#include <stdlib.h>
#include "sync.h"

int fortune_sync(void) {
    printf("\033[34m[INFO]\033[0m Sincronizando repositorio de recetas...\n");

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "mkdir -p /tmp/fortune_sync && "
        "curl -sL -f \"%s\" -o /tmp/fortune_sync/recipes.tar.gz && "
        "mkdir -p %s && "
        "tar -xzf /tmp/fortune_sync/recipes.tar.gz -C %s --strip-components=2 clover-packages-main/packages && "
        "rm -rf /tmp/fortune_sync",
        RECIPES_REPO_URL, RECIPES_DIR, RECIPES_DIR);

    int status = system(cmd);
    if (status == 0) {
        printf("\033[32m[OK]\033[0m Base de recetas actualizada correctamente en %s.\n", RECIPES_DIR);
        return 0;
    } else {
        fprintf(stderr, "\033[31m[ERROR]\033[0m Falló la sincronización de recetas.\n");
        return 1;
    }
}
