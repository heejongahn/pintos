#include <hash.h>
#include "filesys/file.h"

enum page_location {
  DISK,
  SWAP,
  ZERO
};

struct hash s_page_table;

struct s_page {
  uint8_t *uaddr;
  enum page_location location;

  struct {
    /* Info for file page */
    struct file *file;
    off_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
  } file_info;

  /* Info for swap page */

  struct hash_elem h_elem;
};

void s_page_init (void);
bool s_page_insert_file (uint8_t *, struct file *, off_t, uint32_t, uint32_t, bool);
