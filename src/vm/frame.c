#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

void
init_frame () {
  list_init (&frame_table);
}

uint8_t *
allocate_frame (uint8_t *upage) {
  struct frame *f;
  uint8_t *kpage = palloc_get_page (PAL_USER);

  if (kpage != NULL) {
    f = malloc (sizeof (struct frame));

    f->kpage = kpage;
    f->upage = upage;
    f->owner = thread_current();
    list_push_front (&frame_table, &f->elem);
  }

  return kpage;
}
