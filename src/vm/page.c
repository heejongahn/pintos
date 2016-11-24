#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include <stdio.h>

static unsigned hash_func (const struct hash_elem *, void *);
static bool less_func (const struct hash_elem *, const struct hash_elem *, void *);
static bool install_s_page (uint8_t *, uint8_t *, bool);
static bool s_page_load_file (struct s_page *);
static bool s_page_load_zero (struct s_page *);
static bool s_page_load_swap (struct s_page *);

void
s_page_init () {
  hash_init (&s_page_table, &hash_func, &less_func, NULL);
  lock_init (&s_page_lock);
  list_init (&mmap_list);
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
  page->uaddr = pg_round_down (uaddr);
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
  page->uaddr = pg_round_down (uaddr);
  page->location = ZERO;
  page->writable = true;

  lock_acquire(&s_page_lock);
  success = hash_insert (&s_page_table, &page->h_elem) == NULL;
  lock_release(&s_page_lock);

  return success;
}

bool
s_page_insert_swap (uint8_t *uaddr, bool writable, size_t swap_idx) {
  struct s_page *page = malloc (sizeof (struct s_page));
  bool success;

  if (!page) {
    return false;
  }

  page->uaddr = pg_round_down (uaddr);
  page->location = SWAP;
  page->writable = writable;
  page->swap_idx = swap_idx;

  lock_acquire(&s_page_lock);
  success = hash_insert (&s_page_table, &page->h_elem) == NULL;
  lock_release(&s_page_lock);

  return success;
}

bool
s_page_delete (uint8_t *uaddr) {
  struct s_page *page = page_lookup (uaddr);
  return (hash_delete (&s_page_table, &page->h_elem) != NULL);
}

bool
mmap_add (uint8_t *addr, struct file *f) {
  struct mmap_info *m = malloc (sizeof (struct mmap_info));

  m->addr = addr;
  m->file = f;

  list_push_front (&mmap_list, &m->elem);
}

bool
mmap_remove (int mapid) {
  struct list_elem *curr;
  struct mmap_info *m;
  bool found = false;
  int ofs, remaining;

  uint8_t *addr;
  struct file *f;

  for (curr=list_begin(&mmap_list); curr!=list_tail(&mmap_list);
        curr=list_next(curr)) {
    m = list_entry (curr, struct mmap_info, elem);
    if ((m->file)->fd == mapid) {
      f = m->file;
      found = true;
      break;
    }
  }

  if (found) {
    remaining = file_length (f);
    addr = (m->addr);

    for (ofs = 0; ofs < remaining; ofs += PGSIZE) {
      s_page_delete ((unsigned) addr + ofs);
      remaining -= PGSIZE;
    }

    lock_acquire (&filesys_lock);
    file_close (f);
    lock_release (&filesys_lock);

    free (m);

    return true;
  }

  abnormal_exit();
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
    case SWAP:
      success = s_page_load_swap (page);
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

static bool
s_page_load_swap (struct s_page *page) {
  uint8_t *upage = page->uaddr;
  uint8_t *kpage = frame_alloc (upage);

  if (kpage == NULL)
    return false;

  /* Add the page to the process's address space. */
  if (!install_s_page (upage, kpage, true))
    {
      frame_free (kpage);
      return false;
    }

  if (!swap_in (page->swap_idx, upage))
    return false;

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
