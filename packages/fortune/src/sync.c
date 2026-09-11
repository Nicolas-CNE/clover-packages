#include <stdio.h>
#include <stdlib.h>
#include "sync.h"
#include "packsync.h"

int fortune_sync(void) {
    return fortune_sync_repository(RECIPES_REPO_URL, RECIPES_DIR);
}
