#ifndef FORTUNE_PARSER_H
#define FORTUNE_PARSER_H

typedef struct {
    char name[128];
    char version[64];
    char source_url[512];
    char git_url[512];
    char branch_tag[128];
    char binary_url[512];
    char source_type[32]; // "source", "git", "binary", "meta"
    char build_steps[2048];
    char **dependencies;
    int dep_count;
} Recipe;

Recipe *parser_parse_recipe(const char *filepath);
void parser_free_recipe(Recipe *recipe);

#endif