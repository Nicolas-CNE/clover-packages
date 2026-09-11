#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

// Helper para limpiar espacios al inicio y final
static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Helper para remover comillas al inicio y final del valor (ej. "base" -> base)
static void strip_quotes(char *str) {
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"') {
        str[len - 1] = '\0';
        memmove(str, str + 1, len - 1);
    }
}

// Reemplazo iterativo de $VAR y ${VAR}
static void replace_var(char *str, size_t max_len, const char *var_name, const char *value) {
    if (!str || !var_name || !value) return;

    char buffer[2048];
    char ph_braced[128];
    char ph_unbraced[128];

    snprintf(ph_braced, sizeof(ph_braced), "${%s}", var_name);
    snprintf(ph_unbraced, sizeof(ph_unbraced), "$%s", var_name);

    const char *placeholders[2] = { ph_braced, ph_unbraced };

    for (int i = 0; i < 2; i++) {
        char *pos;
        while ((pos = strstr(str, placeholders[i])) != NULL) {
            size_t prefix_len = pos - str;
            size_t ph_len = strlen(placeholders[i]);

            snprintf(buffer, sizeof(buffer), "%.*s%s%s", (int)prefix_len, str, value, pos + ph_len);
            strncpy(str, buffer, max_len - 1);
            str[max_len - 1] = '\0';
        }
    }
}

Recipe *parser_parse_recipe(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "\033[31m[ERROR]\033[0m No se pudo abrir la receta: %s\n", filepath);
        return NULL;
    }

    Recipe *recipe = calloc(1, sizeof(Recipe));
    if (!recipe) {
        fclose(fp);
        return NULL;
    }

    char line[2048];
    int reading_build_steps = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Eliminar salto de línea
        line[strcspn(line, "\r\n")] = 0;

        // Si estamos acumulando las líneas de BUILD_STEPS multilínea
        if (reading_build_steps) {
            char *trimmed = trim_whitespace(line);

            // Cierre explícito mediante una comilla solitaria
            if (strcmp(trimmed, "\"") == 0) {
                reading_build_steps = 0;
                continue;
            }

            // Acumular comando con salto de línea
            if (recipe->build_steps[0] != '\0') {
                strncat(recipe->build_steps, "\n", sizeof(recipe->build_steps) - strlen(recipe->build_steps) - 1);
            }
            
            // Si la línea finaliza el bloque terminando en "
            size_t tlen = strlen(trimmed);
            if (tlen > 0 && trimmed[tlen - 1] == '"' && (tlen == 1 || trimmed[tlen - 2] != '\\')) {
                trimmed[tlen - 1] = '\0';
                strncat(recipe->build_steps, trimmed, sizeof(recipe->build_steps) - strlen(recipe->build_steps) - 1);
                reading_build_steps = 0;
            } else {
                strncat(recipe->build_steps, line, sizeof(recipe->build_steps) - strlen(recipe->build_steps) - 1);
            }
            continue;
        }

        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '#' || trimmed[0] == '\0') continue;

        if (strncmp(trimmed, "NAME=", 5) == 0) {
            snprintf(recipe->name, sizeof(recipe->name), "%s", trimmed + 5);
            strip_quotes(recipe->name);
        } else if (strncmp(trimmed, "VERSION=", 8) == 0) {
            snprintf(recipe->version, sizeof(recipe->version), "%s", trimmed + 8);
            strip_quotes(recipe->version);
        } else if (strncmp(trimmed, "URL=", 4) == 0) {
            snprintf(recipe->source_url, sizeof(recipe->source_url), "%s", trimmed + 4);
            strip_quotes(recipe->source_url);
        } else if (strncmp(trimmed, "GIT_URL=", 8) == 0) {
            snprintf(recipe->git_url, sizeof(recipe->git_url), "%s", trimmed + 8);
            strip_quotes(recipe->git_url);
        } else if (strncmp(trimmed, "BRANCH=", 7) == 0 || strncmp(trimmed, "TAG=", 4) == 0) {
            const char *val = (trimmed[0] == 'B') ? trimmed + 7 : trimmed + 4;
            snprintf(recipe->branch_tag, sizeof(recipe->branch_tag), "%s", val);
            strip_quotes(recipe->branch_tag);
        } else if (strncmp(trimmed, "DOWNLOAD_SIZE=", 14) == 0) {
            snprintf(recipe->download_size, sizeof(recipe->download_size), "%s", trimmed + 14);
            strip_quotes(recipe->download_size);
        } else if (strncmp(trimmed, "INSTALLED_SIZE=", 15) == 0) {
            snprintf(recipe->installed_size, sizeof(recipe->installed_size), "%s", trimmed + 15);
            strip_quotes(recipe->installed_size);
        } else if (strncmp(trimmed, "BUILD_STEPS=", 12) == 0) {
            const char *val = trimmed + 12;
            char *val_trimmed = strdup(val);
            if (val_trimmed) {
                strip_quotes(val_trimmed);
                snprintf(recipe->build_steps, sizeof(recipe->build_steps), "%s", val_trimmed);
                free(val_trimmed);
            }

            // Solo activa multilínea si empieza con comilla y NO termina en comilla en la misma línea
            size_t len = strlen(trimmed);
            if (val[0] == '"' && (len == 13 || trimmed[len - 1] != '"' || (len > 13 && trimmed[len - 2] == '\\'))) {
                reading_build_steps = 1;
            }
        } else if (strncmp(trimmed, "DEPENDENCIES=", 13) == 0 || strncmp(trimmed, "DEPS=", 5) == 0) {
            const char *deps_start = (trimmed[0] == 'D' && trimmed[1] == 'E' && trimmed[2] == 'P' && trimmed[3] == 'S') ? trimmed + 5 : trimmed + 13;
            char *deps_str = strdup(deps_start);
            if (deps_str) {
                strip_quotes(deps_str);
                char *token = strtok(deps_str, " ,");
                while (token) {
                    char **next = realloc(recipe->dependencies, sizeof(char *) * (recipe->dep_count + 1));
                    if (!next) break;
                    recipe->dependencies = next;
                    recipe->dependencies[recipe->dep_count] = strdup(token);
                    recipe->dep_count++;
                    token = strtok(NULL, " ,");
                }
                free(deps_str);
            }
        }
    }

    fclose(fp);

    // Expansión universal de $NAME, ${NAME}, $VERSION, ${VERSION}
    replace_var(recipe->source_url, sizeof(recipe->source_url), "NAME", recipe->name);
    replace_var(recipe->source_url, sizeof(recipe->source_url), "VERSION", recipe->version);
    replace_var(recipe->git_url, sizeof(recipe->git_url), "NAME", recipe->name);
    replace_var(recipe->git_url, sizeof(recipe->git_url), "VERSION", recipe->version);
    replace_var(recipe->branch_tag, sizeof(recipe->branch_tag), "VERSION", recipe->version);

    return recipe;
}

void parser_free_recipe(Recipe *recipe) {
    if (recipe) {
        if (recipe->dependencies) {
            for (int i = 0; i < recipe->dep_count; i++) {
                free(recipe->dependencies[i]);
            }
            free(recipe->dependencies);
        }
        free(recipe);
    }
}
