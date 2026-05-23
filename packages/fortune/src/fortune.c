/*Fortune PM*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

void install_package(const char *pkg_name) {
    char url_base[MAX_URL_LEN] = {0};
    char build_cmd[4096] = {0};
    char tmp_tarball[] = "/tmp/pkg.tar.gz";
    char tmp_dir[] = "/tmp/pkg_src";
    int ret;

    // Determine URL base from pkg_name or alias
    if (strcmp(pkg_name, "cmatrix") == 0) {
        snprintf(url_base, sizeof(url_base), "https://github.com/bisqwit/cmatrix");
    } else {
        // Assume pkg_name is a full GitHub repo URL
        strncpy(url_base, pkg_name, sizeof(url_base) - 1);
    }

    // Clean previous workspace safely
    snprintf(build_cmd, sizeof(build_cmd), "rm -rf %s %s", tmp_dir, tmp_tarball);
    system(build_cmd);

    // Universal single-line fallback loop using curl -f (fails on 404)
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

    if (flag_verbose) {
        printf("[Clover PM] Downloading source code from repository with fallback branches...\n");
        printf("[Clover PM] Running command:\n%s\n", build_cmd);
    }

    system(build_cmd);

    // Robust file size verification (Must exist and be > 100 bytes to avoid garbage)
    snprintf(build_cmd, sizeof(build_cmd),
        "bash -c 'if [ ! -s %s ] || [ $(wc -c < %s) -lt 100 ]; then exit 1; fi'",
        tmp_tarball, tmp_tarball);

    ret = system(build_cmd);
    if (ret != 0) {
        fprintf(stderr, "[Clover PM] Error: Source code could not be downloaded from any known branch (404) or archive is invalid.\n");
        exit(1);
    }

    // Prepare source extraction directory
    snprintf(build_cmd, sizeof(build_cmd), "mkdir -p %s", tmp_dir);
    system(build_cmd);

    // Extract package based on file type header signature or extension
    if (strstr(url_base, ".zip") != NULL || access(tmp_tarball, F_OK) == 0 && system("file /tmp/pkg.tar.gz | grep -q 'Zip'") == 0) {
        snprintf(build_cmd, sizeof(build_cmd), "unzip -q %s -d %s && mv %s/*/* %s/ 2>/dev/null || true", tmp_tarball, tmp_dir, tmp_dir, tmp_dir);
    } else {
        snprintf(build_cmd, sizeof(build_cmd), "tar -xzf %s -C %s --strip-components=1 2>/dev/null || tar -xzf %s -C %s", tmp_tarball, tmp_dir, tmp_tarball, tmp_dir);
    }
    
    if (flag_verbose) {
        printf("[Clover PM] Extracting source archive...\n");
    }
    
    ret = system(build_cmd);
    if (ret != 0) {
        fprintf(stderr, "[Clover PM] Error: Failed to extract source archive.\n");
        exit(1);
    }

    // Change directory to source
    if (chdir(tmp_dir) != 0) {
        perror("[Clover PM] Error: Cannot change directory to source");
        exit(1);
    }

    // Detect build system and build accordingly
    if (access("Makefile", F_OK) == 0) {
        if (flag_verbose) printf("[Clover PM] Detected Makefile, running 'make && make install'\n");
        ret = system("make && make install");
    } else if (access("CMakeLists.txt", F_OK) == 0) {
        if (flag_verbose) printf("[Clover PM] Detected CMakeLists.txt, running cmake build sequence\n");
        ret = system("mkdir -p build && cd build && cmake .. && make && make install");
    } else if (access("install.sh", F_OK) == 0) {
        if (flag_verbose) printf("[Clover PM] Detected install.sh, running it\n");
        ret = system("chmod +x install.sh && ./install.sh");
    } else if (access("autogen.sh", F_OK) == 0 || access("configure", F_OK) == 0) {
        if (flag_verbose) printf("[Clover PM] Detected autogen.sh or configure, running GNU Autotools pipeline\n");
        ret = system("if [ -f autogen.sh ]; then chmod +x autogen.sh && ./autogen.sh; fi && [ -f configure ] && chmod +x configure && ./configure && make && make install");
    } else {
        fprintf(stderr, "[Clover PM] Error: Unknown build system. No Makefile, CMakeLists.txt, install.sh, autogen.sh, or configure found.\n");
        exit(1);
    }

    if (ret != 0) {
        fprintf(stderr, "[Clover PM] Error: Build or installation process failed.\n");
        exit(1);
    }

    // Cleanup workspace
    chdir("/");
    snprintf(build_cmd, sizeof(build_cmd), "rm -rf %s %s", tmp_dir, tmp_tarball);
    system(build_cmd);

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
    (void)recipe_path; (void)key; (void)output; (void)max_len;
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
        install_package(pkg_arg);
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
