#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

void
frame_init () {
  list_init (&frame_table);
  lock_init (&frame_lock);
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

void
free_frame (uint8_t *kpage) {
  struct list_elem *curr;
  struct frame *frame;
  bool found = false;

  for (curr=list_begin(&frame_table); curr!=list_tail(&frame_table);
          curr=list_next(curr)) {
    frame = list_entry(curr, struct frame, elem);
    if (frame->kpage == kpage) {
      found = true;
      break;
    }
  }

  if (found) {
    palloc_free_page (kpage);
    list_remove (curr);
  }
}
