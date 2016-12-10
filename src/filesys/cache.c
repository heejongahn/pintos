#include "filesys/cache.h"

#define BUFFER_CACHE_SIZE 64

static struct lock bcache_lock;
static struct bcache_elem bcache[BUFFER_CACHE_SIZE];

void
bcache_init () {
  lock_init (&bcache_lock);
}
