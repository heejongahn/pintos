#include <stdio.h>
#include "threads/vaddr.h"
#include "vm/swap.h"

void
swap_init () {
  int swap_pages = disk_size (disk_get (1,1)) / (PGSIZE / DISK_SECTOR_SIZE);

  swap_table = bitmap_create (swap_pages);
}
