#include <stdio.h>
#include "threads/vaddr.h"
#include "vm/page.h"
#include "vm/swap.h"

#define SECTOR_PER_PAGE (PGSIZE / DISK_SECTOR_SIZE)

void
swap_init () {
  swap_disk = disk_get (1,1);
  swap_table = bitmap_create (disk_size (swap_disk) / SECTOR_PER_PAGE);
  lock_init (&swap_lock);
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

  lock_acquire (&swap_lock);
  for (i=0; i<SECTOR_PER_PAGE; i++) {
    buffer = (unsigned) uaddr + DISK_SECTOR_SIZE * i;
    disk_read (swap_disk, start_sector + i, buffer);
  }
  lock_release (&swap_lock);
  return true;
}

bool
swap_out (struct s_page *page) {
  int i;
  void *buffer;
  uint8_t *uaddr;

  // Find free swap slot
  size_t swap_idx = bitmap_scan_and_flip (swap_table, 0, 1, 0);
  if (swap_idx == BITMAP_ERROR) {
    return false;
  }

  uaddr = page->uaddr;
  // Actually writing into the disk
  disk_sector_t start_sector = swap_idx * SECTOR_PER_PAGE;

  lock_acquire (&swap_lock);
  for (i=0; i<SECTOR_PER_PAGE; i++) {
    buffer = (unsigned) uaddr + DISK_SECTOR_SIZE * i;
    disk_write (swap_disk, start_sector + i, buffer);
  }
  lock_release (&swap_lock);

  // Add to s_page_table
  hash_delete (&s_page_table, &page->h_elem);
  s_page_insert_swap (page->uaddr, page->writable, swap_idx);

  return true;
}

