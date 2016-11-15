#include "list.h"
#include "threads/synch.h"

static struct list frame_table;
static struct lock frame_lock;

struct frame {
  struct list_elem elem;
  uint8_t *upage;
  uint8_t *kpage;
  struct thread *owner;
  bool pinned;
};

void frame_init (void);

uint8_t *frame_alloc (uint8_t *);
void frame_free (uint8_t *);
uint8_t *frame_evict (void);
struct frame *find_victim (void);
void frame_pin (uint8_t *);
void frame_unpin (uint8_t *);
struct frame *frame_find (uint8_t *);
bool is_frame_allocated (uint8_t *);
