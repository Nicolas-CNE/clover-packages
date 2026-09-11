#define _POSIX_C_SOURCE 200809L

#include "deps.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

DepGraph *deps_create(void) {
    DepGraph *graph = calloc(1, sizeof(DepGraph));
    if (!graph) {
        return NULL;
    }

    graph->capacity = 8;
    graph->nodes = calloc((size_t)graph->capacity, sizeof(DepNode));
    if (!graph->nodes) {
        free(graph);
        return NULL;
    }

    return graph;
}

int deps_find_node(const DepGraph *graph, const char *name) {
    if (!graph || !name) {
        return -1;
    }

    for (int i = 0; i < graph->count; i++) {
        if (strcmp(graph->nodes[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

int deps_add_node(DepGraph *graph, const char *name, char **deps, int dep_count, void *userdata) {
    if (!graph || !name) {
        return -1;
    }

    int existing = deps_find_node(graph, name);
    if (existing >= 0) {
        return existing;
    }

    if (graph->count >= graph->capacity) {
        int new_capacity = graph->capacity * 2;
        DepNode *nodes = realloc(graph->nodes, (size_t)new_capacity * sizeof(DepNode));
        if (!nodes) {
            return -1;
        }
        graph->nodes = nodes;
        graph->capacity = new_capacity;
    }

    DepNode *node = &graph->nodes[graph->count];
    memset(node, 0, sizeof(*node));
    strncpy(node->name, name, sizeof(node->name) - 1);
    node->userdata = userdata;
    node->state = DEPS_UNVISITED;

    if (dep_count > 0 && deps) {
        node->deps = calloc((size_t)dep_count, sizeof(char *));
        if (!node->deps) {
            return -1;
        }

        for (int i = 0; i < dep_count; i++) {
            node->deps[i] = strdup(deps[i]);
            if (!node->deps[i]) {
                for (int j = 0; j < i; j++) {
                    free(node->deps[j]);
                }
                free(node->deps);
                node->deps = NULL;
                node->dep_count = 0;
                return -1;
            }
        }
        node->dep_count = dep_count;
    }

    return graph->count++;
}

void *deps_get_userdata(const DepGraph *graph, const char *name) {
    int idx = deps_find_node(graph, name);
    if (idx < 0) {
        return NULL;
    }
    return graph->nodes[idx].userdata;
}

static int dfs_visit(DepGraph *graph, int idx, char ***queue, int *queue_len, int *queue_cap,
                     char *errbuf, size_t errlen) {
    DepNode *node = &graph->nodes[idx];

    if (node->state == DEPS_VISITING) {
        if (errbuf && errlen > 0) {
            snprintf(errbuf, errlen, "Circular dependency detected involving '%s'", node->name);
        }
        return -1;
    }

    if (node->state == DEPS_VISITED) {
        return 0;
    }

    node->state = DEPS_VISITING;

    for (int i = 0; i < node->dep_count; i++) {

     int dep_idx = deps_find_node(graph, node->deps[i]);
     if (dep_idx < 0) {
     if (errbuf && errlen > 0) {
        snprintf(errbuf, errlen, "Dependencia no resuelta '%s' para el paquete '%s'", node->deps[i], node->name);
     }
     return -1;
     }
        if (dfs_visit(graph, dep_idx, queue, queue_len, queue_cap, errbuf, errlen) != 0) {
            return -1;
        }
    }

    node->state = DEPS_VISITED;

    if (*queue_len >= *queue_cap) {
        int new_cap = (*queue_cap == 0) ? 8 : (*queue_cap * 2);
        char **next = realloc(*queue, (size_t)new_cap * sizeof(char *));
        if (!next) {
            return -1;
        }
        *queue = next;
        *queue_cap = new_cap;
    }

    (*queue)[(*queue_len)++] = strdup(node->name);
    if (!(*queue)[*queue_len - 1]) {
        return -1;
    }

    return 0;
}

int deps_toposort(DepGraph *graph, char ***queue, int *queue_len, char *errbuf, size_t errlen) {
    if (!graph || !queue || !queue_len) {
        return -1;
    }

    for (int i = 0; i < graph->count; i++) {
        graph->nodes[i].state = DEPS_UNVISITED;
    }

    char **result = NULL;
    int len = 0;
    int cap = 0;

    for (int i = 0; i < graph->count; i++) {
        if (graph->nodes[i].state != DEPS_UNVISITED) {
            continue;
        }

        if (dfs_visit(graph, i, &result, &len, &cap, errbuf, errlen) != 0) {
            deps_free_queue(result, len);
            return -1;
        }
    }

    *queue = result;
    *queue_len = len;
    return 0;
}

void deps_free_queue(char **queue, int queue_len) {
    if (!queue) {
        return;
    }

    for (int i = 0; i < queue_len; i++) {
        free(queue[i]);
    }
    free(queue);
}

void deps_free(DepGraph *graph) {
    if (!graph) {
        return;
    }

    for (int i = 0; i < graph->count; i++) {
        DepNode *node = &graph->nodes[i];
        for (int j = 0; j < node->dep_count; j++) {
            free(node->deps[j]);
        }
        free(node->deps);
    }

    free(graph->nodes);
    free(graph);
}
