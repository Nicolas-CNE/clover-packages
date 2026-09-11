#ifndef PACKSYNC_H
#define PACKSYNC_H

// Descarga únicamente el archivo PACKINDEX.txt a /var/lib/fortune/recipes
int fortune_sync_repository(const char *repo_url, const char *target_dir);

// Descarga la receta de un paquete específico si no existe localmente
int fetch_recipe_if_missing(const char *pkg_name, const char *target_dir);

#endif
