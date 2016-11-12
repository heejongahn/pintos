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
