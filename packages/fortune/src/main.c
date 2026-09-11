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
    printf("  search <query>      Search packages in local index\n");
    printf("  install <package>   Build and install a package and its dependencies\n");
    printf("  uninstall <package> Remove an installed package\n");
    printf("  list                List installed packages\n\n");
    printf("Options:\n");
    printf("  -v, --verbose       Enable detailed logging\n");
    printf("  -h, --help          Show this help\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    // Detectar flag verbose (-v o --verbose)
    int verbose = 0;
    int arg_offset = 1;

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--verbose") == 0) {
        verbose = 1;
        arg_offset = 2;
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }
    }

    const char *cmd = argv[arg_offset];

    // Adquirir cerrojo de base de datos
    int lock_fd = db_lock();
    if (lock_fd < 0) {
        return 1;
    }

    if (strcmp(cmd, "sync") == 0) {
        fortune_sync();
    } else if (strcmp(cmd, "search") == 0 && argc >= (arg_offset + 2)) {
        cmd_search(argv[arg_offset + 1]);
    } else if (strcmp(cmd, "install") == 0 && argc >= (arg_offset + 2)) {
        core_install(argv[arg_offset + 1], verbose);
    } else if (strcmp(cmd, "uninstall") == 0 && argc >= (arg_offset + 2)) {
        core_uninstall(argv[arg_offset + 1]);
    } else if (strcmp(cmd, "list") == 0) {
        core_list_installed();
    } else {
        print_usage(argv[0]);
    }

    // Liberar cerrojo de base de datos
    db_unlock(lock_fd);
    return 0;
}
