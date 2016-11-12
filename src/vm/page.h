#include "hash.h"

enum page_location {
  FRAME,
  DISK,
  SWAP,
  ZERO
};

struct hash s_page_table;

struct s_page {
  uint8_t *uaddr;
  enum page_location location;

  struct hash_elem h_elem;
};

void s_page_init (void);
