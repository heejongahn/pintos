#include <stdio.h>
#include "threads/vaddr.h"
#include "vm/swap.h"

#define SECTOR_PER_PAGE (PGSIZE / DISK_SECTOR_SIZE)

void
swap_init () {
  swap_disk = disk_get (1,1);
  swap_table = bitmap_create (disk_size (swap_disk) / SECTOR_PER_PAGE);
}

bool
swap_out (struct s_page *page) {
  int i;
  void *buffer;

  // Find free swap slot
  size_t swap_idx = bitmap_scan_and_flip (swap_table, 0, 1, 0);
  if (swap_idx == BITMAP_ERROR) {
    return false;
  }

  // Actually writing into the disk
  disk_sector_t start_sector = swap_idx * SECTOR_PER_PAGE;

  for (i=0; i<SECTOR_PER_PAGE; i++) {
    buffer = page->uaddr + (DISK_SECTOR_SIZE / sizeof (uint8_t *));
    disk_write (swap_disk, start_sector + i, buffer);
  }

  // Add to s_page_table
  s_page_insert_swap (page->uaddr, page->writable, swap_idx);

  return true;
}

bool
swap_in (size_t swap_idx, uint8_t *uaddr) {
  int i;
  void *buffer;

  if (!bitmap_test (swap_table, swap_idx)) {
    return false;
  }

  bitmap_flip (swap_table, swap_idx);
  disk_sector_t start_sector = swap_idx * SECTOR_PER_PAGE;

  for (i=0; i<SECTOR_PER_PAGE; i++) {
    buffer = uaddr + (DISK_SECTOR_SIZE / sizeof (uint8_t *));
    disk_read (swap_disk, start_sector + i, buffer);
  }

  return true;
}
