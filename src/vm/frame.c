#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/swap.h"

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
  } else {
    kpage = frame_evict ();
    f = frame_find (kpage);
    f->upage = upage;
    f->owner = thread_current();
  }

  return kpage;
}

/*
 * Evict a frame from frame table.
 * Swap in, free frame, and clear page table in order.
 */
uint8_t *
frame_evict () {
  struct frame *f = find_victim ();
  struct thread *t = f->owner;

  struct s_page *page = page_lookup (f->upage);

  if (page == NULL) {
    printf ("no such page\n");
    return NULL;
  }

  // First, add evicting page to swap table
  if (!swap_out (page)) {
    return NULL;
  }

  // Remove from page table
  pagedir_clear_page (t->pagedir, f->upage);

  return f->kpage;
}

void
frame_free (uint8_t *kpage) {
  struct frame *f = frame_find (kpage);

  if (f != NULL) {
    palloc_free_page (kpage);
    list_remove (&f->elem);
  } else {
    printf ("Trying to free not allocated frame.\n");
    abnormal_exit ();
  }
}

/*
 * Find victim frame.
 * Currently just return the last frame
 */
struct frame *
find_victim () {
  struct list_elem *e = list_rbegin (&frame_table);
  return list_entry (e, struct frame, elem);
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
  struct frame *f = frame_find (kpage);
  return (f != NULL);
}
