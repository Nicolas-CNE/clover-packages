#define _POSIX_C_SOURCE 200809L

#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#define LOCK_FILE "/var/lib/fortune/fortune.lock"

static bool is_valid_pkg_name(const char *name) {
    if (!name || strlen(name) == 0 || strlen(name) > 128) return false;
    for (int i = 0; name[i]; i++) {
        if (!isalnum(name[i]) && name[i] != '-' && name[i] != '_') return false;
    }
    return true;
}

int db_init(void) {
    struct stat st = {0};
    const char *db_dir = "/var/lib/fortune";
    if (stat(db_dir, &st) == -1) {
        if (mkdir(db_dir, 0755) != 0 && errno != EEXIST) {
            return -1;
        }
    }
    return 0;
}

int db_lock(void) {
    db_init();

    int fd = open(LOCK_FILE, O_CREAT | O_RDWR | O_CLOEXEC, 0666);
    if (fd < 0) {
        if (errno == EACCES || errno == EPERM) {
            fprintf(stderr, "\033[31m[ERROR]\033[0m Permiso denegado al intentar abrir el lockfile. Ejecuta con sudo.\n");
        } else {
            perror("\033[31m[ERROR]\033[0m No se pudo abrir el lockfile");
        }
        return -1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            fprintf(stderr, "\033[31m[ERROR]\033[0m Otra instancia de Fortune PM está ejecutándose.\n");
        } else {
            perror("\033[31m[ERROR]\033[0m Error al aplicar flock");
        }
        close(fd);
        return -1;
    }

    return fd;
}

void db_unlock(int fd) {
    if (fd >= 0) {
        flock(fd, LOCK_UN);
        close(fd);
        unlink(LOCK_FILE);
    }
}

bool db_is_installed(const char *pkg_name) {
    if (!is_valid_pkg_name(pkg_name)) return false;
    char path[512];
    snprintf(path, sizeof(path), "/var/lib/fortune/%s.manifest", pkg_name);
    return (access(path, F_OK) == 0);
}

int db_register_package(const PackageRecord *pkg) {
    if (!pkg || !is_valid_pkg_name(pkg->name)) return -1;
    char path[512];
    snprintf(path, sizeof(path), "/var/lib/fortune/%s.manifest", pkg->name);

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "NAME=%s\n", pkg->name);
    fprintf(f, "VERSION=%s\n", pkg->version);
    fprintf(f, "DESCRIPTION=%s\n", pkg->description);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char date_buf[32];
    if (t) {
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", t);
        fprintf(f, "INSTALL_DATE=%s\n", date_buf);
    }

    fprintf(f, "FILES_COUNT=%d\n", pkg->file_count);
    fprintf(f, "FILES_LIST:\n");
    for (int i = 0; i < pkg->file_count; i++) {
        if (pkg->files[i]) {
            fprintf(f, "%s\n", pkg->files[i]);
        }
    }

    fclose(f);
    return 0;
}

bool is_safe_path(const char *path) {
    const char *safe_prefixes[] = {
        "/usr/bin/", "/usr/share/", "/usr/local/",
        "/usr/lib/", "/opt/", "/var/lib/fortune/"
    };
    int num_prefixes = (int)(sizeof(safe_prefixes) / sizeof(safe_prefixes[0]));

    for (int i = 0; i < num_prefixes; i++) {
        if (strncmp(path, safe_prefixes[i], strlen(safe_prefixes[i])) == 0) {
            return true;
        }
    }
    return false;
}

int db_unregister_package(const char *pkg_name) {
    if (!is_valid_pkg_name(pkg_name)) return -1;

    PackageRecord *pkg = db_get_package(pkg_name);
    if (!pkg) return -1;

    printf("[FortunePM] Desinstalando %s...\n", pkg->name);

    for (int i = 0; i < pkg->file_count; i++) {
        if (pkg->files[i] && pkg->files[i][0] == '/') {
            if (is_safe_path(pkg->files[i])) {
                if (unlink(pkg->files[i]) == 0) {
                    printf("  Borrado: %s\n", pkg->files[i]);
                } else if (errno != ENOENT) {
                    perror("  Error al borrar");
                }
            } else {
                printf("  [ALERTA DE SEGURIDAD] Se bloqueó el borrado de la ruta protegida: %s\n", pkg->files[i]);
            }
        }
    }

    char path[512];
    snprintf(path, sizeof(path), "/var/lib/fortune/%s.manifest", pkg_name);
    int ret = unlink(path);

    db_free_record(pkg);
    return ret;
}

PackageRecord *db_get_package(const char *pkg_name) {
    if (!is_valid_pkg_name(pkg_name)) return NULL;
    char path[512];
    snprintf(path, sizeof(path), "/var/lib/fortune/%s.manifest", pkg_name);

    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    PackageRecord *pkg = calloc(1, sizeof(PackageRecord));
    if (!pkg) {
        fclose(f);
        return NULL;
    }

    char line[1024];
    bool in_files = false;
    int file_idx = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strncmp(line, "NAME=", 5) == 0) {
            strncpy(pkg->name, line + 5, sizeof(pkg->name) - 1);
        } else if (strncmp(line, "VERSION=", 8) == 0) {
            strncpy(pkg->version, line + 8, sizeof(pkg->version) - 1);
        } else if (strncmp(line, "DESCRIPTION=", 12) == 0) {
            strncpy(pkg->description, line + 12, sizeof(pkg->description) - 1);
        } else if (strncmp(line, "INSTALL_DATE=", 13) == 0) {
            strncpy(pkg->install_date, line + 13, sizeof(pkg->install_date) - 1);
        } else if (strncmp(line, "FILES_COUNT=", 12) == 0) {
            pkg->file_count = atoi(line + 12);
            if (pkg->file_count > 0 && pkg->file_count < 100000) {
                pkg->files = calloc(pkg->file_count, sizeof(char *));
            }
        } else if (strcmp(line, "FILES_LIST:") == 0) {
            in_files = true;
        } else if (in_files && pkg->files && file_idx < pkg->file_count) {
            pkg->files[file_idx++] = strdup(line);
        }
    }

    if (in_files) {
        pkg->file_count = file_idx; // Sincronizar el número real de archivos parseados
    }

    fclose(f);
    return pkg;
}

void db_free_record(PackageRecord *pkg) {
    if (!pkg) return;
    if (pkg->files) {
        for (int i = 0; i < pkg->file_count; i++) {
            if (pkg->files[i]) free(pkg->files[i]);
        }
        free(pkg->files);
    }
    free(pkg);
}