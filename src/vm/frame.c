#include "vm/frame.h"
#include "threads/malloc.h"

void
init_frame () {
  list_init (&frame_table);
}

void
allocate_frame (uint8_t *kpage, uint8_t *upage) {
  struct frame *f = malloc (sizeof (struct frame));

  f->kpage = kpage;
  f->upage = upage;
  f->owner = thread_current();
  list_push_front (&frame_table, &f->elem);
}
