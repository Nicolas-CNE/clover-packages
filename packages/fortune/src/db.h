#ifndef FORTUNE_DB_H
#define FORTUNE_DB_H

#include <stdbool.h>

typedef struct {
    char name[256];
    char version[64];
    char description[512];
    char install_date[32];
    char **files;
    int file_count;
} PackageRecord;

int db_init(void);
int db_lock(void);
void db_unlock(int lock_fd);
bool db_is_installed(const char *pkg_name);
int db_register_package(const PackageRecord *pkg);
int db_unregister_package(const char *pkg_name);
PackageRecord *db_get_package(const char *pkg_name);
void db_free_record(PackageRecord *pkg);
bool is_safe_path(const char *path);

#endif
