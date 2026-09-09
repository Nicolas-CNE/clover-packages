#ifndef FORTUNE_DEPS_H
#define FORTUNE_DEPS_H

#include <stddef.h>

typedef enum {
    DEPS_UNVISITED,
    DEPS_VISITING,
    DEPS_VISITED
} DepState;

typedef struct DepNode {
    char name[128];
    char **deps;
    int dep_count;
    DepState state;
    void *userdata;
} DepNode;

typedef struct DepGraph {
    DepNode *nodes;
    int count;
    int capacity;
} DepGraph;

DepGraph *deps_create(void);
void deps_free(DepGraph *graph);

int deps_find_node(const DepGraph *graph, const char *name);
int deps_add_node(DepGraph *graph, const char *name, char **deps, int dep_count, void *userdata);
void *deps_get_userdata(const DepGraph *graph, const char *name);

int deps_toposort(DepGraph *graph, char ***queue, int *queue_len, char *errbuf, size_t errlen);
void deps_free_queue(char **queue, int queue_len);

#endif
