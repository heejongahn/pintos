#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"

void
frame_init () {
  list_init (&frame_table);
  lock_init (&frame_lock);
}

uint8_t *
frame_alloc (uint8_t *upage) {
  struct frame *f;
  uint8_t *kpage = palloc_get_page (PAL_USER);

  if (kpage != NULL) {
    f = malloc (sizeof (struct frame));

    f->kpage = kpage;
    f->upage = upage;
    f->owner = thread_current();
    f->pinned = false;

    list_push_front (&frame_table, &f->elem);
  }

  return kpage;
}

void
frame_free (uint8_t *kpage) {
  bool found = false;
  struct frame *f = frame_find (kpage);

  if (f != NULL) {
    palloc_free_page (kpage);
    list_remove (&f->elem);
  } else {
    printf ("Trying to free not allocated frame.\n");
    abnormal_exit ();
  }
}

void
frame_pin (uint8_t *kpage) {
  struct frame *f = frame_find (kpage);

  if (f != NULL) {
    f->pinned = true;
  }
}


void
frame_unpin (uint8_t *kpage) {
  struct frame *f = frame_find (kpage);

  if (f != NULL) {
    f->pinned = false;
  }
}

struct frame *
frame_find (uint8_t *kpage) {
  struct list_elem *curr;
  struct frame *frame;

  for (curr=list_begin(&frame_table); curr!=list_tail(&frame_table);
          curr=list_next(curr)) {
    frame = list_entry(curr, struct frame, elem);
    if (frame->kpage == kpage) {
      return frame;
    }
  }
  return NULL;
}

bool
is_frame_allocated (uint8_t *kpage) {
  return frame_find (kpage) != NULL;
}
