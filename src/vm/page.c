#include "threads/malloc.h"
#include "vm/page.h"

static unsigned
hash_func (const struct hash_elem *element, void *aux) {
  struct s_page *s_page = hash_entry(element, struct s_page, h_elem);
  return hash_int (s_page->uaddr);
}

static bool
less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
  struct s_page *s_page_a = hash_entry(a, struct s_page, h_elem);
  struct s_page *s_page_b = hash_entry(b, struct s_page, h_elem);
  return ((s_page_a->uaddr) < (s_page_b->uaddr));
}

void
s_page_init () {
  hash_init (&s_page_table, &hash_func, &less_func, NULL);
}

bool
s_page_insert_file (uint8_t *uaddr, struct file *file, off_t ofs,
    uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
  struct s_page *s_page = malloc (sizeof (struct s_page));

  s_page->uaddr = uaddr;
  s_page->location = DISK;

  s_page->file_info.file = file;
  s_page->file_info.ofs = ofs;
  s_page->file_info.read_bytes = read_bytes;
  s_page->file_info.zero_bytes = zero_bytes;
  s_page->file_info.writable = writable;

  struct hash_elem *success = hash_insert (&s_page_table, &s_page->h_elem);

  if (!success) {
    return true;
  }

  return false;
}
