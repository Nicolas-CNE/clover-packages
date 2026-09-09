#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "db.h"
#include "sync.h"

void print_usage(const char *prog_name) {
    printf("Usage: %s [options] <command> [<package>]\n\n", prog_name);
    printf("Commands:\n");
    printf("  sync                Sync remote recipe repositories\n");
    printf("  install <package>   Install a software package\n");
    printf("  uninstall <package> Remove an installed package\n");
    printf("  list                List installed packages\n");
    printf("  pkg-build <package> Build standalone package binary\n\n");
    printf("Options:\n");
    printf("  -v, --verbose        Enable detailed logging\n");
    printf("  -h, --help           Show this help\n");
}

int main(int argc, char *argv[]) {
    // Si no se pasaron argumentos o pidieron ayuda, salimos SIN pedir lock
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    // Pedimos el lock
    int lock_fd = db_lock();
    if (lock_fd < 0) {
        return 1;
    }

    // Ejecutamos el comando
    if (strcmp(argv[1], "install") == 0 && argc >= 3) {
        core_install(argv[2]);
    } else if (strcmp(argv[1], "uninstall") == 0 && argc >= 3) {
        core_uninstall(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        core_list_installed();
    } else {
        print_usage(argv[0]);
    }

    // IMPRESCINDIBLE: Liberar SIEMPRE el lock antes de terminar
    db_unlock(lock_fd);
    return 0;
}
