#ifndef FORTUNE_CORE_H
#define FORTUNE_CORE_H

int core_pkg_build(const char *pkg_name);
int core_verify_privileges(void);
int core_install(const char *pkg_name, int verbose);
void core_install_bin(const char *pkg_name);
void core_uninstall(const char *pkg_name);
void core_list_installed(void);
int core_sync_repo(void);

#endif
