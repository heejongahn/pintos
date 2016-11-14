#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "vm/frame.h"
#include <stdio.h>

static unsigned hash_func (const struct hash_elem *, void *);
static bool less_func (const struct hash_elem *, const struct hash_elem *, void *);
static bool install_s_page (uint8_t *, uint8_t *, bool);

void
s_page_init () {
  hash_init (&s_page_table, &hash_func, &less_func, NULL);
  lock_init (&s_page_lock);
}

bool
s_page_insert_file (uint8_t *uaddr, struct file *file, off_t ofs,
    uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
  struct s_page *page = malloc (sizeof (struct s_page));
  if (!page) {
    return false;
  }

  printf ("s_page_insert_file at %p\n", uaddr);
  page->uaddr = uaddr;
  page->location = DISK;

  page->file_info.file = file;
  page->file_info.ofs = ofs;
  page->file_info.read_bytes = read_bytes;
  page->file_info.zero_bytes = zero_bytes;
  page->file_info.writable = writable;

  lock_acquire(&s_page_lock);
  struct hash_elem *success = hash_insert (&s_page_table, &page->h_elem);
  lock_release(&s_page_lock);

  if (!success) {
    return true;
  }

  return false;
}

bool
s_page_load_file (struct s_page *page) {
  uint8_t *upage = page->uaddr;
  struct file *file = page->file_info.file;
  off_t ofs = page->file_info.ofs;
  uint32_t read_bytes = page->file_info.read_bytes;
  uint32_t zero_bytes = page->file_info.zero_bytes;
  bool writable = page->file_info.writable;

  printf ("s_page_load_file at %p\n", upage);
  lock_acquire(&filesys_lock);
  file_seek (file, ofs);
  lock_release(&filesys_lock);

  /* Get a page of memory. */
  uint8_t *kpage = allocate_frame (upage);
  if (kpage == NULL)
    return false;

  lock_acquire(&filesys_lock);
  /* Load this page. */
  if (file_read (file, kpage, read_bytes) != (int) read_bytes)
    {
      palloc_free_page (kpage);
      lock_release(&filesys_lock);
      return false;
    }
  lock_release(&filesys_lock);
  memset (kpage + read_bytes, 0, zero_bytes);

  /* Add the page to the process's address space. */
  if (!install_s_page (upage, kpage, writable))
    {
      palloc_free_page (kpage);
      return false;
    }

  return true;
}

struct s_page *
page_lookup (const void *uaddr) {
  struct s_page page;
  struct hash_elem *h_elem;

  page.uaddr = pg_round_down (uaddr);
  h_elem = hash_find (&s_page_table, &page.h_elem);
  return h_elem != NULL ? hash_entry (h_elem, struct s_page, h_elem) : NULL;
}

static bool
install_s_page (uint8_t *upage, uint8_t *kpage, bool writable) {
  struct thread *t = thread_current();

  return (pagedir_get_page (t->pagedir, upage) == NULL
      && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

static unsigned
hash_func (const struct hash_elem *element, void *aux) {
  struct s_page *page = hash_entry(element, struct s_page, h_elem);
  return hash_int (page->uaddr);
}

static bool
less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
  struct s_page *page_a = hash_entry(a, struct s_page, h_elem);
  struct s_page *page_b = hash_entry(b, struct s_page, h_elem);
  return ((page_a->uaddr) < (page_b->uaddr));
}
