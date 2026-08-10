/*Fortune PM*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>

/* Constants */
#define INSTALL_PATH "/usr/bin"
#define MAX_URL_LEN 1024
#define MAX_NAME_LEN 256
#define REPO_BASE_URL "https://raw.githubusercontent.com/Nicolas-CNE/clover-packages/main/packages"
#define INDEX_URL "https://raw.githubusercontent.com/Nicolas-CNE/clover-packages/main/index.txt"
#define CACHE_DIR "/tmp/clover_cache"

/* Global Flags */
int flag_force = 0;
int flag_verbose = 0;
int flag_yes = 0;

/* Function Prototypes */
void install_package(const char *pkg_name);
void uninstall_package(const char *pkg_name);
void list_packages();
void update_repository();
void search_packages(const char *keyword);
int is_root();
int get_recipe_value(const char *recipe_path, const char *key, char *output, size_t max_len);
int confirm_action(const char *message);
void resolve_and_install(const char *pkg_name);
int fetch_clover_recipe(const char *pkg_name);
int run_cmd_check_cancel(const char *cmd);

/* see if the lil bitch ctrl c'ed it */
int run_cmd_check_cancel(const char *cmd) {
    int status = system(cmd);
    
    if (WIFSIGNALED(status) && (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGQUIT)) {
        fprintf(stderr, "\n[Clover PM] Operación cancelada por el usuario.\n");
        exit(EXIT_FAILURE);
    }
    
    if (WIFEXITED(status) && WEXITSTATUS(status) == 130) {
        fprintf(stderr, "\n[Clover PM] Operación cancelada por el usuario.\n");
        exit(EXIT_FAILURE);
    }

    return status;
}

int fetch_clover_recipe(const char *pkg_name) {
    char build_cmd[1024];
    char recipe_path[256];
    snprintf(recipe_path, sizeof(recipe_path), "/tmp/%s.recipe", pkg_name);
    snprintf(build_cmd, sizeof(build_cmd), 
             "curl -sL -f -o %s " REPO_BASE_URL "/%s/%s.recipe 2>/dev/null", 
             recipe_path, pkg_name, pkg_name);
    
    return (run_cmd_check_cancel(build_cmd) == 0);
}

void resolve_and_install(const char *pkg_name) {
    if (!is_root()) {
        fprintf(stderr, "[Clover PM] Error: Necesitás permisos de root (sudo) para instalar.\n");
        exit(EXIT_FAILURE);
    }

    // clover on top
    if (fetch_clover_recipe(pkg_name)) {
        if (flag_verbose) printf("[Clover PM] Receta encontrada en clover-packages. Compilando...\n");
        install_package(pkg_name);
        return;
    }

    char build_cmd[2048];

    // fallback to arch repo
    if (flag_verbose) printf("[Clover PM] Buscando en repositorios oficiales de Arch Linux...\n");
    
    snprintf(build_cmd, sizeof(build_cmd),
             "mkdir -p /tmp/clover_abs && cd /tmp/clover_abs && "
             "rm -rf %s && "
             "git clone --depth 1 https://gitlab.archlinux.org/archlinux/packaging/packages/%s.git 2>/dev/null",
             pkg_name, pkg_name);

    if (run_cmd_check_cancel(build_cmd) == 0) {
        if (flag_verbose) printf("[Clover PM] Clonado desde repos oficiales. Ejecutando makepkg...\n");
        
        snprintf(build_cmd, sizeof(build_cmd),
                 "cd /tmp/clover_abs/%s && "
                 "chown -R 1000:1000 . && "
                 "runuser -u $(id -un 1000) -- makepkg -si --noconfirm --needed",
                 pkg_name);
                 
        if (run_cmd_check_cancel(build_cmd) == 0) {
            printf("[Clover PM] '%s' instalado con éxito desde Arch Oficial.\n", pkg_name);
            return;
        } else {
            // if clone ended but not ctrl c then uhh idk man i forgor
            fprintf(stderr, "[Clover PM] Error: La compilación de '%s' falló.\n", pkg_name);
            exit(EXIT_FAILURE);
        }
    }

    // fallback to aur
    if (flag_verbose) printf("[Clover PM] No estaba en el oficial. Buscando en AUR...\n");

    snprintf(build_cmd, sizeof(build_cmd),
             "mkdir -p /tmp/clover_abs && cd /tmp/clover_abs && "
             "rm -rf %s && "
             "git clone --depth 1 https://aur.archlinux.org/%s.git 2>/dev/null",
             pkg_name, pkg_name);

    if (run_cmd_check_cancel(build_cmd) == 0) {
        if (flag_verbose) printf("[Clover PM] Clonado desde AUR. Ejecutando makepkg...\n");
        
        snprintf(build_cmd, sizeof(build_cmd),
                 "cd /tmp/clover_abs/%s && "
                 "chown -R 1000:1000 . && "
                 "runuser -u $(id -un 1000) -- makepkg -si --noconfirm --needed",
                 pkg_name);
                 
        if (run_cmd_check_cancel(build_cmd) == 0) {
            printf("[Clover PM] '%s' instalado con éxito desde AUR.\n", pkg_name);
            return;
        } else {
            fprintf(stderr, "[Clover PM] Error: La compilación desde AUR para '%s' falló.\n", pkg_name);
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr, "[Clover PM] Error: No se encontró '%s' en Clover, Arch Oficial ni AUR.\n", pkg_name);
    exit(EXIT_FAILURE);
}

void install_package(const char *pkg_name) {
    char url_base[MAX_URL_LEN] = {0};
    char build_cmd[4096] = {0};
    char tmp_tarball[] = "/tmp/pkg.tar.gz";
    char tmp_dir[] = "/tmp/pkg_src";
    char recipe_path[256];
    int ret;

    snprintf(recipe_path, sizeof(recipe_path), "/tmp/%s.recipe", pkg_name);

    if (strstr(pkg_name, "http://") || strstr(pkg_name, "https://")) {
        strncpy(url_base, pkg_name, sizeof(url_base) - 1);
    } else {
        if (!get_recipe_value(recipe_path, "URL", url_base, sizeof(url_base))) {
            fprintf(stderr, "[Clover PM] Error: No se pudo leer la clave 'URL' en %s.\n", recipe_path);
            exit(1);
        }
    }

    if (strstr(url_base, ".tar.gz") || strstr(url_base, ".zip")) {
        snprintf(build_cmd, sizeof(build_cmd), "curl -L -s -A \"Mozilla/5.0\" -f -o %s \"%s\"", tmp_tarball, url_base);
    } else {
        snprintf(build_cmd, sizeof(build_cmd),
            "bash -c '"
            "SUCCESS=1; "
            "for branch in master main develop trunk; do "
            "  if curl -L -s -A \"Mozilla/5.0\" -f -o %s \"%s/archive/refs/heads/${branch}.tar.gz\"; then "
            "    SUCCESS=0; break; "
            "  fi; "
            "done; "
            "if [ $SUCCESS -ne 0 ]; then "
            "  curl -L -s -A \"Mozilla/5.0\" -f -o %s \"%s/archive/refs/heads/master.zip\"; "
            "fi'",
            tmp_tarball, url_base, tmp_tarball, url_base);
    }

    if (flag_verbose) {
        printf("[Clover PM] Downloading source code from: %s\n", url_base);
    }

    run_cmd_check_cancel(build_cmd);

    snprintf(build_cmd, sizeof(build_cmd),
        "bash -c 'if [ ! -s %s ] || [ $(wc -c < %s) -lt 100 ]; then exit 1; fi'",
        tmp_tarball, tmp_tarball);

    ret = run_cmd_check_cancel(build_cmd);
    if (ret != 0) {
        fprintf(stderr, "[Clover PM] Error: Source code download failed or file invalid.\n");
        exit(1);
    }

    snprintf(build_cmd, sizeof(build_cmd), "mkdir -p %s", tmp_dir);
    run_cmd_check_cancel(build_cmd);

    if (strstr(url_base, ".zip") != NULL) {
        snprintf(build_cmd, sizeof(build_cmd), "unzip -q %s -d %s && mv %s/*/* %s/ 2>/dev/null || true", tmp_tarball, tmp_dir, tmp_dir, tmp_dir);
    } else {
        snprintf(build_cmd, sizeof(build_cmd), "tar -xzf %s -C %s --strip-components=1 2>/dev/null || tar -xzf %s -C %s", tmp_tarball, tmp_dir, tmp_dir, tmp_dir);
    }
    
    run_cmd_check_cancel(build_cmd);

    if (chdir(tmp_dir) != 0) {
        perror("[Clover PM] Error changing directory");
        exit(1);
    }

    if (access("CMakeLists.txt", F_OK) == 0) {
        if (flag_verbose) printf("[Clover PM] Running CMake & Ninja build sequence...\n");
        ret = run_cmd_check_cancel("mkdir -p build && cd build && cmake -G Ninja .. && ninja && ninja install");
    } else if (access("Makefile", F_OK) == 0) {
        if (flag_verbose) printf("[Clover PM] Running Make...\n");
        ret = run_cmd_check_cancel("make && make install");
    } else {
        fprintf(stderr, "[Clover PM] Error: Unknown build system.\n");
        exit(1);
    }

    if (ret != 0) {
        fprintf(stderr, "[Clover PM] Error: Build or installation failed.\n");
        exit(1);
    }

    chdir("/");
    snprintf(build_cmd, sizeof(build_cmd), "rm -rf %s %s %s", tmp_dir, tmp_tarball, recipe_path);
    run_cmd_check_cancel(build_cmd);

    printf("[Clover PM] Installation of '%s' completed successfully.\n", pkg_name);
}

void uninstall_package(const char *pkg_name) {
    (void)pkg_name;
    printf("[Clover PM] Uninstall function not implemented yet.\n");
}

void list_packages() {
    printf("[Clover PM] List packages function not implemented yet.\n");
}

void update_repository() {
    printf("[Clover PM] Update repository function not implemented yet.\n");
}

void search_packages(const char *keyword) {
    (void)keyword;
    printf("[Clover PM] Search packages function not implemented yet.\n");
}

int is_root() {
    return (geteuid() == 0);
}

int get_recipe_value(const char *recipe_path, const char *key, char *output, size_t max_len) {
    FILE *fp = fopen(recipe_path, "r");
    if (!fp) return 0;

    char line[512];
    size_t key_len = strlen(key);

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, key, key_len) == 0 && line[key_len] == '=') {
            char *val = line + key_len + 1;
            val[strcspn(val, "\r\n")] = 0;
            strncpy(output, val, max_len - 1);
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

int confirm_action(const char *message) {
    if (flag_yes) return 1;
    printf("%s [y/N]: ", message);
    fflush(stdout);
    char response[8];
    if (fgets(response, sizeof(response), stdin) == NULL) return 0;
    return (response[0] == 'y' || response[0] == 'Y');
}

int main(int argc, char *argv[]) {
    int opt;
    char *pkg_arg = NULL;

    while ((opt = getopt(argc, argv, "fvy")) != -1) {
        switch (opt) {
            case 'f': flag_force = 1; break;
            case 'v': flag_verbose = 1; break;
            case 'y': flag_yes = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-fvy] <command> [args]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-fvy] <command> [args]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *command = argv[optind++];
    if (optind < argc) {
        pkg_arg = argv[optind];
    }

    if (strcmp(command, "install") == 0) {
        if (!pkg_arg) {
            fprintf(stderr, "Error: No package specified for install.\n");
            exit(EXIT_FAILURE);
        }
        resolve_and_install(pkg_arg);
    } else if (strcmp(command, "uninstall") == 0) {
        if (!pkg_arg) {
            fprintf(stderr, "Error: No package specified for uninstall.\n");
            exit(EXIT_FAILURE);
        }
        uninstall_package(pkg_arg);
    } else if (strcmp(command, "list") == 0) {
        list_packages();
    } else if (strcmp(command, "update") == 0) {
        update_repository();
    } else if (strcmp(command, "search") == 0) {
        if (!pkg_arg) {
            fprintf(stderr, "Error: No keyword specified for search.\n");
            exit(EXIT_FAILURE);
        }
        search_packages(pkg_arg);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'.\n", command);
        exit(EXIT_FAILURE);
    }

    return 0;
}
