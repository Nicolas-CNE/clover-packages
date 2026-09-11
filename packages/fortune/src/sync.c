#include <stdio.h>
#include <stdlib.h>
#include "sync.h"
#include "packsync.h"

// En el handler del comando sync:
const char *repo = "https://github.com/Nicolas-CNE/clover-packages/archive/refs/heads/main.tar.gz";
const char *db_path = "/var/lib/fortune/recipes";

fortune_sync_repository(repo, db_path);

int fortune_sync(void) {
    printf("[INFO] Sincronizando repositorio de recetas desde %s...\n", RECIPES_REPO_URL);

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "mkdir -p /tmp/fortune_sync && "
        "curl -fsSL \"%s\" -o /tmp/fortune_sync/recipes.tar.gz && "
        "mkdir -p %s && "
        "tar -xzf /tmp/fortune_sync/recipes.tar.gz -C %s --strip-components=1 && "
        "rm -rf /tmp/fortune_sync",
        RECIPES_REPO_URL, RECIPES_DIR, RECIPES_DIR);

    int status = system(cmd);
    if (status == 0) {
        printf("[INFO] Base de recetas actualizada correctamente en %s.\n", RECIPES_DIR);
        return 0;
    } else {
        // Limpieza por si quedó la carpeta temporal ante un fallo
        system("rm -rf /tmp/fortune_sync");
        fprintf(stderr, "[ERROR] Falló la descarga o extracción del repositorio de recetas.\n");
        return 1;
    }
}
