#include "list.h"

struct list frame_table;

struct frame {
  struct list_elem elem;
  uint8_t *upage;
  uint8_t *kpage;
  struct thread *owner;
};

void init_frame (void);
uint8_t *allocate_frame (uint8_t *);