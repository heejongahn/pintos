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
static bool s_page_load_file (struct s_page *);
static bool s_page_load_zero (struct s_page *);

void
s_page_init () {
  hash_init (&s_page_table, &hash_func, &less_func, NULL);
  lock_init (&s_page_lock);
}

bool
s_page_insert_file (uint8_t *uaddr, struct file *file, off_t ofs,
    uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
  struct s_page *page = malloc (sizeof (struct s_page));
  bool success;
  if (!page) {
    return false;
  }

  // printf ("s_page_insert_file at %p\n", uaddr);
  page->uaddr = uaddr;
  page->location = DISK;
  page->writable = writable;

  page->file_info.file = file;
  page->file_info.ofs = ofs;
  page->file_info.read_bytes = read_bytes;
  page->file_info.zero_bytes = zero_bytes;

  lock_acquire(&s_page_lock);
  success = hash_insert (&s_page_table, &page->h_elem) == NULL;
  lock_release(&s_page_lock);

  return success;
}

bool
s_page_insert_zero (uint8_t *uaddr) {
  struct s_page *page = malloc (sizeof (struct s_page));
  bool success;

  if (!page) {
    return false;
  }

  // printf ("s_page_insert_zero at %p\n", uaddr);
  page->uaddr = uaddr;
  page->location = ZERO;
  page->writable = true;

  lock_acquire(&s_page_lock);
  success = hash_insert (&s_page_table, &page->h_elem) == NULL;
  lock_release(&s_page_lock);

  return success;
}

bool
s_page_insert_swap (uint8_t *uaddr, bool writable, unsigned swap_ofs) {
  struct s_page *page = malloc (sizeof (struct s_page));
  bool success;

  if (!page) {
    return false;
  }

  page->uaddr = uaddr;
  page->location = SWAP;
  page->writable = writable;
  page->swap_ofs = swap_ofs;

  lock_acquire(&s_page_lock);
  success = hash_insert (&s_page_table, &page->h_elem) == NULL;
  lock_release(&s_page_lock);

  return success;
}

bool
s_page_load (struct s_page *page) {
  bool success = true;

  switch (page->location) {
    case DISK:
      success = s_page_load_file (page);
      break;
    case ZERO:
      success = s_page_load_zero (page);
      break;
  }

  return success;
}

static bool
s_page_load_file (struct s_page *page) {
  uint8_t *upage = page->uaddr;
  struct file *file = page->file_info.file;
  off_t ofs = page->file_info.ofs;
  uint32_t read_bytes = page->file_info.read_bytes;
  uint32_t zero_bytes = page->file_info.zero_bytes;
  bool writable = page->writable;

  // printf ("s_page_load_file at %p\n", upage);
  lock_acquire(&filesys_lock);
  file_seek (file, ofs);
  lock_release(&filesys_lock);

  /* Get a page of memory. */
  uint8_t *kpage = frame_alloc (upage);
  if (kpage == NULL)
    return false;

  lock_acquire(&filesys_lock);
  /* Load this page. */
  if (file_read (file, kpage, read_bytes) != (int) read_bytes)
    {
      frame_free (kpage);
      lock_release(&filesys_lock);
      return false;
    }
  lock_release(&filesys_lock);
  memset (kpage + read_bytes, 0, zero_bytes);

  /* Add the page to the process's address space. */
  if (!install_s_page (upage, kpage, writable))
    {
      frame_free (kpage);
      return false;
    }

  return true;
}

static bool
s_page_load_zero (struct s_page *page) {
  uint8_t *upage = page->uaddr;

  // printf ("s_page_load_zero at %p\n", upage);

  /* Get a page of memory. */
  uint8_t *kpage = frame_alloc (upage);
  if (kpage == NULL)
    return false;

  memset (kpage, 0, PGSIZE);

  /* Add the page to the process's address space. */
  if (!install_s_page (upage, kpage, true))
    {
      frame_free (kpage);
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

bool
is_stack_access (void *addr, void *esp) {
  return (esp - 32 <= addr) && is_user_vaddr(addr);
}

bool
grow_stack (void *addr) {
  s_page_insert_zero(pg_round_down(addr));
  return s_page_load (page_lookup (addr));
}
