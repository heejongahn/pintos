#include "list.h"
#include "threads/synch.h"

static struct list frame_table;
static struct lock frame_lock;

struct frame {
  struct list_elem elem;
  uint8_t *upage;
  uint8_t *kpage;
  struct thread *owner;
};

void init_frame (void);
uint8_t *allocate_frame (uint8_t *);
