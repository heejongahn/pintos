#include <hash.h>
#include "threads/synch.h"
#include "filesys/file.h"

enum page_location {
  DISK,
  SWAP,
  ZERO
};

struct hash s_page_table;
struct lock s_page_lock;
struct list mmap_list;

struct mmap_info {
  uint8_t *addr;
  struct file *file;

  struct list_elem elem;
};

struct s_page {
  uint8_t *uaddr;
  enum page_location location;
  bool writable;

  struct {
    /* Info for file page */
    struct file *file;
    off_t ofs;
    uint32_t read_bytes;
    uint32_t zero_bytes;
  } file_info;

  /* Info for swap page */
  size_t swap_idx;

  struct hash_elem h_elem;
};

void s_page_init (void);
bool s_page_insert_file (uint8_t *, struct file *, off_t, uint32_t, uint32_t, bool);
bool s_page_insert_zero (uint8_t *);
bool s_page_insert_swap (uint8_t *, bool, unsigned);
bool s_page_delete (uint8_t *);

bool s_page_load (struct s_page *);
struct s_page *page_lookup (const void *);

bool is_stack_access (void *, void *);
bool grow_stack (void *addr);
