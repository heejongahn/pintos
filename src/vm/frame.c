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
  struct frame *f = find_victim();
  struct thread *t;
  struct s_page *page;

  while (f == NULL) {
    f = find_victim ();
  }

  t = f->owner;

  page = page_lookup (f->upage);

  if (page == NULL) {
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
  struct frame* victim = list_entry (e, struct frame, elem);

  /* Second chance */
  uint32_t *pd = (victim->owner)->pagedir;
  void *page = victim->upage;

  if (pagedir_is_accessed (pd, page) || victim->pinned) {
    pagedir_set_accessed (pd, page, false);
    list_remove (&victim->elem);
    list_push_front (&frame_table, &victim->elem);

    victim = NULL;
  }

  return victim;
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
