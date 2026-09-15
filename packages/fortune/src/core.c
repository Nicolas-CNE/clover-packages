#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ftw.h>
#include "core.h"
#include "parser.h"
#include "network.h"
#include "db.h"
#include "packsync.h"
#include "deps.h"

#define RECIPES_DIR "/var/lib/fortune/recipes"

static int run_command(const char *cmd) {
    printf("\033[90m[EXEC]\033[0m %s\n", cmd);
    int res = system(cmd);
    return WEXITSTATUS(res);
}

int core_verify_privileges(void) {
    if (geteuid() != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m Este comando requiere permisos de superusuario (root).\n");
        return -1;
    }
    return 0;
}

// Resuelve la ruta relativa real leyendo PACKINDEX.txt o buscando en fallback
static int resolve_recipe_path(const char *pkg_name, char *out_path, size_t out_size) {
    FILE *f = fopen(RECIPES_DIR "/PACKINDEX.txt", "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            char name[128], version[64], desc[256], rel_path[256];
            if (sscanf(line, "%127[^|]|%63[^|]|%255[^|]|%255[^\n]", name, version, desc, rel_path) == 4) {
                if (strcmp(name, pkg_name) == 0) {
                    snprintf(out_path, out_size, "%s/%s", RECIPES_DIR, rel_path);
                    fclose(f);
                    if (access(out_path, F_OK) == 0) return 0;
                }
            }
        }
        fclose(f);
    }

    // Fallbacks
    snprintf(out_path, out_size, "%s/%s/%s.recipe", RECIPES_DIR, pkg_name, pkg_name);
    if (access(out_path, F_OK) == 0) return 0;

    snprintf(out_path, out_size, "%s/packages/%s/%s.recipe", RECIPES_DIR, pkg_name, pkg_name);
    if (access(out_path, F_OK) == 0) return 0;

    return -1;
}

static void resolve_work_directory(const char *base_build, const Recipe *r, char *target_dir, size_t target_size) {
    char test_path[1024];
    snprintf(test_path, sizeof(test_path), "%s/configure", base_build);
    
    if (access(test_path, F_OK) == 0) {
        snprintf(target_dir, target_size, "%s", base_build);
        return;
    }

    snprintf(test_path, sizeof(test_path), "%s/%s-%s", base_build, r->name, r->version);
    struct stat st;
    if (stat(test_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(target_dir, target_size, "%s", test_path);
        return;
    }

    DIR *d = opendir(base_build);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] != '.') {
                snprintf(test_path, sizeof(test_path), "%s/%s", base_build, dir->d_name);
                if (stat(test_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    snprintf(target_dir, target_size, "%s", test_path);
                    closedir(d);
                    return;
                }
            }
        }
        closedir(d);
    }

    snprintf(target_dir, target_size, "%s", base_build);
}

static int enter_build_directory(const char *work_dir) {
    if (chdir(work_dir) != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo acceder a: %s\n", work_dir);
        return -1;
    }

    DIR *d = opendir(".");
    if (!d) return 0;

    struct dirent *dir;
    char single_subdir[512] = {0};
    int count = 0;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        if (dir->d_type == DT_DIR) {
            count++;
            snprintf(single_subdir, sizeof(single_subdir), "%s", dir->d_name);
        }
    }
    closedir(d);

    if (count == 1 && strlen(single_subdir) > 0) {
        if (chdir(single_subdir) != 0) {
            fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo entrar a: %s\n", single_subdir);
            return -1;
        }
    }

    return 0;
}

static int execute_recipe_build(const Recipe *r, const char *work_dir, const char *fakeroot) {
    char cmd[4096];

    if (strcmp(r->source_type, "meta") == 0 || strcmp(r->source_type, "none") == 0) {
        printf("\033[34m[INFO]\033[0m Paquete meta/vacío detectado. Omitiendo compilación.\n");
        return 0;
    }

    if (enter_build_directory(work_dir) != 0) {
        return -1;
    }

    if (r->build_steps[0] != '\0') {
        char script_path[512];
        snprintf(script_path, sizeof(script_path), "%s/../fortune_build.sh", fakeroot);

        FILE *f = fopen(script_path, "w");
        if (!f) {
            fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo crear el script de compilación.\n");
            return -1;
        }

        fprintf(f, "#!/bin/sh\nset -e\n%s\n", r->build_steps);
        fclose(f);

        chmod(script_path, 0755);

        snprintf(cmd, sizeof(cmd), "export DESTDIR=\"%s\" && \"%s\"", fakeroot, script_path);
        int res = run_command(cmd);

        unlink(script_path);
        return res;
    }

    if (strcmp(r->source_type, "cmake") == 0 || access("CMakeLists.txt", F_OK) == 0) {
        snprintf(cmd, sizeof(cmd), "cmake -B build -DCMAKE_INSTALL_PREFIX=/usr && cmake --build build && DESTDIR=\"%s\" cmake --install build", fakeroot);
        return run_command(cmd);
    } 

    if (strcmp(r->source_type, "autotools") == 0 || access("configure", F_OK) == 0) {
        snprintf(cmd, sizeof(cmd), "./configure --prefix=/usr && make -j$(nproc) && make DESTDIR=\"%s\" install", fakeroot);
        return run_command(cmd);
    }

    if (strcmp(r->source_type, "makefile") == 0 || access("Makefile", F_OK) == 0) {
        snprintf(cmd, sizeof(cmd), "make -j$(nproc) && make DESTDIR=\"%s\" install", fakeroot);
        return run_command(cmd);
    }

    fprintf(stderr, "\033[31m[ERROR]\033[0m Sistema de compilación no reconocido o BUILD_STEPS faltantes.\n");
    return -1;
}

static PackageRecord *current_rec_scan = NULL;
static const char *current_fakeroot_prefix = NULL;

static int scan_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb; (void)typeflag;

    if (ftwbuf->level == 0) return 0;

    const char *rel_path = fpath;
    if (current_fakeroot_prefix != NULL && strncmp(fpath, current_fakeroot_prefix, strlen(current_fakeroot_prefix)) == 0) {
        rel_path += strlen(current_fakeroot_prefix);
    }

    if (strlen(rel_path) == 0) rel_path = "/";

    char **tmp = realloc(current_rec_scan->files, sizeof(char *) * (current_rec_scan->file_count + 1));
    if (!tmp) return -1;

    current_rec_scan->files = tmp;
    current_rec_scan->files[current_rec_scan->file_count] = strdup(rel_path);
    current_rec_scan->file_count++;

    return 0;
}

static void scan_and_populate_files(const char *fakeroot_path, PackageRecord *rec) {
    rec->files = NULL;
    rec->file_count = 0;
    current_rec_scan = rec;
    current_fakeroot_prefix = fakeroot_path;

    nftw(fakeroot_path, scan_callback, 20, FTW_PHYS);
}

static void core_install_single(Recipe *r) {
    char tmp_dir[] = "/tmp/fortune_XXXXXX";
    if (!mkdtemp(tmp_dir)) return;

    char build_dir[512], actual_work_dir[1024], archive[512], fakeroot[512], cmd[4096];
    snprintf(build_dir, sizeof(build_dir), "%s/build", tmp_dir);
    snprintf(archive, sizeof(archive), "%s/source.tmp", tmp_dir);
    snprintf(fakeroot, sizeof(fakeroot), "%s/fakeroot", tmp_dir);

    mkdir(build_dir, 0755);
    mkdir(fakeroot, 0755);

    if (strlen(r->git_url) > 0) {
        printf("\033[34m[INFO]\033[0m Clonando repositorio Git %s...\n", r->git_url);
        if (strlen(r->branch_tag) > 0) {
            snprintf(cmd, sizeof(cmd), "git clone --depth 1 --branch %s %s %s", r->branch_tag, r->git_url, build_dir);
        } else {
            snprintf(cmd, sizeof(cmd), "git clone --depth 1 %s %s", r->git_url, build_dir);
        }
        if (run_command(cmd) != 0) return;
    } else if (strlen(r->source_url) > 0 && strcmp(r->source_url, "none") != 0) {
        printf("\033[34m[INFO]\033[0m Descargando %s...\n", r->source_url);
        if (net_download_file(r->source_url, archive) != 0) return;
        snprintf(cmd, sizeof(cmd), "tar -xf %s -C %s --strip-components=1 2>/dev/null || tar -xf %s -C %s", archive, build_dir, archive, build_dir);
        run_command(cmd);
    }

    resolve_work_directory(build_dir, r, actual_work_dir, sizeof(actual_work_dir));
    chdir(actual_work_dir);

    printf("\033[34m[INFO]\033[0m Compilando %s en %s...\n", r->name, actual_work_dir);
    int install_res = execute_recipe_build(r, actual_work_dir, fakeroot);

    if (install_res != 0) {
        chdir("/tmp");
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmp_dir);
        run_command(cmd);
        return;
    }

    PackageRecord rec;
    memset(&rec, 0, sizeof(PackageRecord));
    snprintf(rec.name, sizeof(rec.name), "%s", r->name);
    snprintf(rec.version, sizeof(rec.version), "%s", r->version);

    scan_and_populate_files(fakeroot, &rec);

    snprintf(cmd, sizeof(cmd), "cp -r %s/* / 2>/dev/null || true", fakeroot);
    run_command(cmd);
    db_register_package(&rec);

    if (rec.files) {
        for (int i = 0; i < rec.file_count; i++) free(rec.files[i]);
        free(rec.files);
    }

    printf("\033[32m[SUCCESS]\033[0m %s %s instalado correctamente.\n", r->name, r->version);
    chdir("/tmp");
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmp_dir);
    run_command(cmd);
}

static int populate_dep_graph(DepGraph *graph, const char *pkg_name) {
    if (deps_find_node(graph, pkg_name) >= 0) return 0;

    if (fetch_recipe_if_missing(pkg_name, RECIPES_DIR) != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo obtener la receta para: %s\n", pkg_name);
        return -1;
    }

    char recipe_path[512];
    if (resolve_recipe_path(pkg_name, recipe_path, sizeof(recipe_path)) != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se encontró el archivo de receta para: %s\n", pkg_name);
        return -1;
    }

    Recipe *r = parser_parse_recipe(recipe_path);
    if (!r) return -1;

 deps_add_node(graph, r->name, (char **)r->dependencies, r->dep_count, NULL);

    for (int i = 0; i < r->dep_count; i++) {
        if (populate_dep_graph(graph, r->dependencies[i]) != 0) {
            parser_free_recipe(r);
            return -1;
        }
    }

    parser_free_recipe(r);
    return 0;
}

void core_install(const char *pkg_name, int verbose) {
    if (core_verify_privileges() != 0) return;

    DepGraph *graph = deps_create();
    if (!graph) return;

    if (populate_dep_graph(graph, pkg_name) != 0) {
        deps_free(graph);
        return;
    }

    char **install_queue = NULL;
    int queue_len = 0;
    char errbuf[256];

    if (deps_toposort(graph, &install_queue, &queue_len, errbuf, sizeof(errbuf)) != 0) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m Error al ordenar dependencias: %s\n", errbuf);
        deps_free(graph);
        return;
    }

    for (int i = 0; i < queue_len; i++) {
        const char *current_pkg = install_queue[i];

        if (db_is_installed(current_pkg)) {
            if (verbose) printf("[INFO] El paquete '%s' ya está instalado, omitiendo.\n", current_pkg);
            continue;
        }

        char recipe_path[512];
        if (resolve_recipe_path(current_pkg, recipe_path, sizeof(recipe_path)) != 0) {
            fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo encontrar la receta para: %s\n", current_pkg);
            deps_free_queue(install_queue, queue_len);
            deps_free(graph);
            return;
        }

        Recipe *r = parser_parse_recipe(recipe_path);
        if (r) {
            core_install_single(r);
            parser_free_recipe(r);
        }
    }

    deps_free_queue(install_queue, queue_len);
    deps_free(graph);
}

void core_uninstall(const char *pkg_name) {
    if (core_verify_privileges() != 0) return;
    db_unregister_package(pkg_name);
}

void core_list_installed(void) {
    printf("\033[34m[INFO]\033[0m Listando paquetes instalados...\n");

    DIR *d = opendir("/var/lib/fortune");
    if (!d) {
        printf("  (Ningún paquete instalado aún)\n");
        return;
    }

    struct dirent *dir;
    int count = 0;

    while ((dir = readdir(d)) != NULL) {
        char *ext = strrchr(dir->d_name, '.');
        if (ext && strcmp(ext, ".manifest") == 0) {
            char pkg_name[256];
            size_t len = ext - dir->d_name;
            snprintf(pkg_name, sizeof(pkg_name), "%.*s", (int)len, dir->d_name);

            PackageRecord *pkg = db_get_package(pkg_name);
            if (pkg) {
                printf("  • \033[1m%s\033[0m v%s (%d archivos)\n", pkg->name, pkg->version, pkg->file_count);
                db_free_record(pkg);
                count++;
            }
        }
    }

    closedir(d);

    if (count == 0) {
        printf("  (Ningún paquete instalado aún)\n");
    }
}
