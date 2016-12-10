#include "filesys/cache.h"

#define BUFFER_CACHE_SIZE 64

static struct lock bcache_lock;
static struct bcache_elem bcache[BUFFER_CACHE_SIZE];

void
bcache_init () {
  lock_init (&bcache_lock);
}

// Find empty bcache. If none is available, return NULL.
struct bcache_elem *
bcache_find_empty () {
  int i;
  struct bcache_elem *e = NULL;

  for (i=0; i++; i<BUFFER_CACHE_SIZE) {
    e = &bcache[i];
    if (!e->valid) {
      break;
    }
  }

  return e;
}
