#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

static char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static void replace_var(char *str, size_t max_len, const char *var_name, const char *value) {
    char buffer[1024];
    char placeholder[64];
    snprintf(placeholder, sizeof(placeholder), "${%s}", var_name);

    char *pos = strstr(str, placeholder);
    if (pos) {
        size_t prefix_len = pos - str;
        snprintf(buffer, sizeof(buffer), "%.*s%s%s", (int)prefix_len, str, value, pos + strlen(placeholder));
        strncpy(str, buffer, max_len - 1);
        str[max_len - 1] = '\0';
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
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;

        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '#' || trimmed[0] == '\0') continue;

        if (strncmp(trimmed, "NAME=", 5) == 0) {
            snprintf(recipe->name, sizeof(recipe->name), "%s", trimmed + 5);
        } else if (strncmp(trimmed, "VERSION=", 8) == 0) {
            snprintf(recipe->version, sizeof(recipe->version), "%s", trimmed + 8);
        } else if (strncmp(trimmed, "URL=", 4) == 0) {
            snprintf(recipe->source_url, sizeof(recipe->source_url), "%s", trimmed + 4);
        } else if (strncmp(trimmed, "GIT_URL=", 8) == 0) {
            snprintf(recipe->git_url, sizeof(recipe->git_url), "%s", trimmed + 8);
        } else if (strncmp(trimmed, "BRANCH=", 7) == 0 || strncmp(trimmed, "TAG=", 4) == 0) {
            const char *val = (trimmed[0] == 'B') ? trimmed + 7 : trimmed + 4;
            snprintf(recipe->branch_tag, sizeof(recipe->branch_tag), "%s", val);
        } else if (strncmp(trimmed, "SOURCE_TYPE=", 12) == 0) {
            snprintf(recipe->source_type, sizeof(recipe->source_type), "%s", trimmed + 12);
        } else if (strncmp(trimmed, "BUILD_STEPS=", 12) == 0) {
            snprintf(recipe->build_steps, sizeof(recipe->build_steps), "%s", trimmed + 12);
        } else if (strncmp(trimmed, "DOWNLOAD_SIZE=", 14) == 0) {
            snprintf(recipe->download_size, sizeof(recipe->download_size), "%s", trimmed + 14);
        } else if (strncmp(trimmed, "INSTALLED_SIZE=", 15) == 0) {
            snprintf(recipe->installed_size, sizeof(recipe->installed_size), "%s", trimmed + 15);
        } else if (strncmp(trimmed, "DEPENDENCIES=", 13) == 0 || strncmp(trimmed, "DEPS=", 5) == 0) {
            const char *deps_start = (trimmed[0] == 'D' && trimmed[1] == 'E' && trimmed[2] == 'P' && trimmed[3] == 'S') ? trimmed + 5 : trimmed + 13;
            char *deps_str = strdup(deps_start);
            if (deps_str) {
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

    // Expandir variables ${NAME} y ${VERSION} en las URLs
    replace_var(recipe->source_url, sizeof(recipe->source_url), "NAME", recipe->name);
    replace_var(recipe->source_url, sizeof(recipe->source_url), "VERSION", recipe->version);
    replace_var(recipe->git_url, sizeof(recipe->git_url), "NAME", recipe->name);

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
