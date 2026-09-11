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

// Verificar si la ruta coincide exactamente con un directorio crítico del sistema
static bool is_system_root_dir(const char *path) {
    const char *protected_dirs[] = {
        "/", "/bin", "/sbin", "/etc", "/lib", "/lib64", "/usr", 
        "/usr/bin", "/usr/sbin", "/usr/lib", "/usr/lib64", 
        "/usr/include", "/usr/share", "/usr/share/man", "/var",
        "/var/lib", "/opt", NULL
    };

    for (int i = 0; protected_dirs[i] != NULL; i++) {
        if (strcmp(path, protected_dirs[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Validar que el nombre del paquete contenga solo caracteres alfanuméricos, guiones o guión bajo
static bool is_valid_pkg_name(const char *name) {
    if (!name || strlen(name) == 0 || strlen(name) > 128) return false;
    for (int i = 0; name[i]; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '-' && name[i] != '_') return false;
    }
    return true;
}

// Crear directorio de la base de datos si no existe
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

// Adquirir bloqueo exclusivo (flock)
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

// Liberar bloqueo y eliminar el archivo lockfile
void db_unlock(int fd) {
    if (fd >= 0) {
        flock(fd, LOCK_UN);
        close(fd);
        unlink(LOCK_FILE);
    }
}

// Comprobar si el manifiesto del paquete existe en el sistema
bool db_is_installed(const char *pkg_name) {
    if (!is_valid_pkg_name(pkg_name)) return false;
    char path[512];
    snprintf(path, sizeof(path), "/var/lib/fortune/%s.manifest", pkg_name);
    return (access(path, F_OK) == 0);
}

// Guardar los datos y la lista de archivos del paquete en su archivo .manifest
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

// Validar que la ruta sea segura para instalar/desinstalar y no sea un directorio protegido
bool is_safe_path(const char *path) {
    if (!path || path[0] != '/') return false;

    // Si es un directorio raíz/base de sistema, bloquear borrado
    if (is_system_root_dir(path)) {
        return false;
    }

    // Prefijos válidos autorizados
    const char *safe_prefixes[] = {
        "/bin/", "/sbin/", "/lib/", "/lib64/",
        "/usr/", "/opt/", "/etc/", "/var/"
    };
    size_t num_prefixes = sizeof(safe_prefixes) / sizeof(safe_prefixes[0]);

    for (size_t i = 0; i < num_prefixes; i++) {
        if (strncmp(path, safe_prefixes[i], strlen(safe_prefixes[i])) == 0) {
            return true;
        }
    }

    return false;
}

// Eliminar archivos, limpiar carpetas vacías y remover el .manifest
int db_unregister_package(const char *pkg_name) {
    if (!is_valid_pkg_name(pkg_name)) return -1;

    PackageRecord *pkg = db_get_package(pkg_name);
    if (!pkg) return -1;

    printf("[FortunePM] Desinstalando %s...\n", pkg->name);

    // PASE 1: Eliminar archivos y symlinks
    for (int i = 0; i < pkg->file_count; i++) {
        const char *file_path = pkg->files[i];
        if (file_path && file_path[0] == '/') {
            if (is_safe_path(file_path)) {
                struct stat st;
                if (lstat(file_path, &st) == 0) {
                    if (S_ISDIR(st.st_mode)) {
                        continue; // Omitir directorios en el primer pase
                    }
                    if (unlink(file_path) == 0) {
                        printf("  Borrado: %s\n", file_path);
                    } else if (errno != ENOENT) {
                        perror("  Error al borrar");
                    }
                }
            } else {
                printf("  [ALERTA DE SEGURIDAD] Se bloqueó el borrado de la ruta protegida: %s\n", file_path);
            }
        }
    }

    // PASE 2: Limpiar directorios vacíos en orden inverso
    for (int i = pkg->file_count - 1; i >= 0; i--) {
        const char *file_path = pkg->files[i];
        if (file_path && file_path[0] == '/' && !is_system_root_dir(file_path)) {
            struct stat st;
            if (lstat(file_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                rmdir(file_path); // Solo borra si quedó vacío
            }
        }
    }

    char path[512];
    snprintf(path, sizeof(path), "/var/lib/fortune/%s.manifest", pkg_name);
    int ret = unlink(path);

    db_free_record(pkg);
    return ret;
}

// Leer y parsear el manifiesto de un paquete instalado
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
            snprintf(pkg->name, sizeof(pkg->name), "%s", line + 5);
        } else if (strncmp(line, "VERSION=", 8) == 0) {
            snprintf(pkg->version, sizeof(pkg->version), "%s", line + 8);
        } else if (strncmp(line, "DESCRIPTION=", 12) == 0) {
            snprintf(pkg->description, sizeof(pkg->description), "%s", line + 12);
        } else if (strncmp(line, "INSTALL_DATE=", 13) == 0) {
            snprintf(pkg->install_date, sizeof(pkg->install_date), "%s", line + 13);
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
        pkg->file_count = file_idx; // Sincronizar el número real de archivos leídos
    }

    fclose(f);
    return pkg;
}

// Liberar memoria asignada dinámicamente a PackageRecord
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
