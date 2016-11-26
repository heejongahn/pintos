#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "vm/page.h"

static bool check_uaddr (void *);
static void **syscall_esp;

static void syscall_handler (struct intr_frame *);
static void get_user (const uint8_t *uaddr, void *save_to, size_t size);
static void put_user (const uint8_t *uaddr, void *copy_from, size_t size);

typedef void (*handler) (void *, uint32_t *, uint32_t *);
typedef int mapid_t;
#define MAP_FAILED ((mapid_t) -1)

static void halt (void **argv, uint32_t *eax, uint32_t *esp);
static void exit (void **argv, uint32_t *eax, uint32_t *esp);
static void exec (void **argv, uint32_t *eax, uint32_t *esp);
static void wait (void **argv, uint32_t *eax, uint32_t *esp);
static void create (void **argv, uint32_t *eax, uint32_t *esp);
static void remove (void **argv, uint32_t *eax, uint32_t *esp);
static void open (void **argv, uint32_t *eax, uint32_t *esp);
static void filesize (void **argv, uint32_t *eax, uint32_t *esp);
static void read (void **argv, uint32_t *eax, uint32_t *esp);
static void write (void **argv, uint32_t *eax, uint32_t *esp);
static void seek (void **argv, uint32_t *eax, uint32_t *esp);
static void tell (void **argv, uint32_t *eax, uint32_t *esp);
static void close (void **argv, uint32_t *eax, uint32_t *esp);
static void mmap (void **argv, uint32_t *eax, uint32_t *esp);
static void munmap (void **argv, uint32_t *eax, uint32_t *esp);

static handler handlers[15] = {
  &halt,
  &exit,
  &exec,
  &wait,
  &create,
  &remove,
  &open,
  &filesize,
  &read,
  &write,
  &seek,
  &tell,
  &close,
  &mmap,
  &munmap
};

/* Check and if UADDR is invalid address, return true
   Return false otherwise */
static bool
check_uaddr (void *uaddr) {
  if ((!uaddr) || is_kernel_vaddr(uaddr)) {
    return true;
  }

  if (!pagedir_get_page(thread_current()->pagedir, uaddr)) {
    if (page_lookup (uaddr)) {
      s_page_load (page_lookup (uaddr));
      return false;
    }

    if (is_stack_access (uaddr, syscall_esp)) {
      grow_stack (uaddr);
      syscall_esp = uaddr;
      return false;
    }

    return true;
  }

  return false;
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void
abnormal_exit (void)
{
  int *abnormal_exit_argv[1] = {-1};
  exit (abnormal_exit_argv, NULL, NULL);
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t *esp = f->esp;
  uint32_t *eax = &f->eax;
  uint32_t syscall_nr;
  int argc, i;
  void *argv[3];

  syscall_esp = f->esp;

  uint32_t *saved_esp = esp;

  memset (argv, 0, sizeof (void *) * 4);
  get_user(esp, &syscall_nr, 4);

  switch (syscall_nr) {
    case SYS_HALT:
      argc = 0;
      break;
    case SYS_EXIT:
    case SYS_EXEC:
    case SYS_WAIT:
    case SYS_REMOVE:
    case SYS_OPEN:
    case SYS_FILESIZE:
    case SYS_TELL:
    case SYS_CLOSE:
    case SYS_MUNMAP:
      argc = 1;
      break;
    case SYS_CREATE:
    case SYS_SEEK:
    case SYS_MMAP:
      argc = 2;
      break;
    case SYS_READ:
    case SYS_WRITE:
      argc = 3;
      break;
    default:
      printf("Unavailable system call for now\n");
      return;
  }
  esp += 1;

  for (i=0; i<argc; i++) {
    get_user((esp+i), &(argv[i]), 4);
  }

  (*handlers[syscall_nr]) (argv, eax, saved_esp);

  return;
}

/* Reads SIZE bytes at user virtual address UADDR.
   Power off if failed. */
static void
get_user (const uint8_t *uaddr, void *save_to, size_t size)
{
  uint8_t *check = uaddr;
  for (check; check < uaddr + size; check++) {
    if (check_uaddr(check)) {
      abnormal_exit();
    }
  }


  memcpy (save_to, uaddr, size);
}

/* Writes SIZE bytes to user address UADDR, copying from COPY_FROM.
   Power off if failed. */
static void
put_user (const uint8_t *uaddr, void *copy_from, size_t size)
{
  uint8_t *check = uaddr;
  for (check; check < uaddr + size; check++) {
    if (check_uaddr(check)) {
      abnormal_exit();
    }
  }

  memcpy (uaddr, copy_from, size);
}


static void
halt (void **argv, uint32_t *eax, uint32_t *esp) {
  power_off();
  return;
}

static void
exit (void **argv, uint32_t *eax, uint32_t *esp) {
  struct thread *t = thread_current ();
  enum intr_level old_level;

  t->exit_code = (int) argv[0];
  printf("%s: exit(%d)\n",
      thread_current()->name, thread_current()->exit_code);
  sema_up(&t->exit_sema);

  sema_down(&t->wait_sema);
  thread_exit();
  return;
}

static void
exec (void **argv, uint32_t *eax, uint32_t *esp) {
  char *cmd_line = (char *) argv[0];
  char *exec_cmd_line;

  if (check_uaddr(cmd_line)) {
    abnormal_exit();
  }

  exec_cmd_line = palloc_get_page(0);
  strlcpy (exec_cmd_line, cmd_line, PGSIZE);
  *eax = process_execute(exec_cmd_line);
  palloc_free_page(exec_cmd_line);
  return;
}

static void
wait (void **argv, uint32_t *eax, uint32_t *esp) {
  tid_t tid = (tid_t) argv[0];

  *eax = process_wait (tid);
  return;
}

static void
create (void **argv, uint32_t *eax, uint32_t *esp) {
  char *file = (char *) argv[0];
  unsigned initial_size = (unsigned) argv[1];

  if (check_uaddr(file)) {
    abnormal_exit();
  }

  lock_acquire (&filesys_lock);
  *eax = filesys_create(file, initial_size);
  lock_release (&filesys_lock);
  return;
}

static void
remove (void **argv, uint32_t *eax, uint32_t *esp) {
  char *name = (char *) argv[0];

  if (check_uaddr(name)) {
    abnormal_exit();
  }

  lock_acquire (&filesys_lock);
  *eax = filesys_remove(name);
  lock_release (&filesys_lock);
  return;
}

static void
open (void **argv, uint32_t *eax, uint32_t *esp) {
  char *cmd_name = (char *) argv[0];

  if (check_uaddr(cmd_name)) {
    abnormal_exit();
  }

  char *name = palloc_get_page(0);
  struct file *f;

  strlcpy (name, cmd_name, PGSIZE);

  lock_acquire(&filesys_lock);
  f = filesys_open (name);
  lock_release(&filesys_lock);

  if (!f) {
    *eax = -1;
  } else {
    *eax = (f->fd);
  }

  palloc_free_page (name);
  return;
}

static void
filesize (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];

  struct file *f = thread_find_file(fd);
  *eax = file_length (f);

  return;
}

static void
read (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];
  void *buffer = (void *) argv[1];
  unsigned size = (unsigned) argv[2];
  int i = 0;
  int stdin_size = 0;

  struct file *f;
  char c;

  int remaining = size;
  unsigned page_buffer = buffer;
  unsigned nxt_boundary;

  int read_bytes;
  struct frame *frame;

  if (fd == 0) {
    for (i; i<size; i++) {
      c = input_getc();
      if (c) {
        stdin_size++;
        ((char *) buffer)[i] = c;
      } else {
        *eax = stdin_size;
        return;
      }
    }
    return;
  }

  if (fd == 1) {
    abnormal_exit();
  }

  f = thread_find_file(fd);

  if (!f) {
    abnormal_exit();
  }

  *eax = 0;
  while (remaining > 0) {
    if (check_uaddr(page_buffer)) {
      abnormal_exit();
    }

    lock_acquire(&filesys_lock);
    if (page_buffer % PGSIZE == 0) {
      read_bytes = remaining > PGSIZE ? PGSIZE : remaining;
    } else {
      nxt_boundary = pg_round_up (page_buffer);
      read_bytes = remaining > (nxt_boundary - page_buffer) ?
        (nxt_boundary - page_buffer) : remaining;
    }

    frame = frame_find (pagedir_get_page (thread_current()->pagedir, page_buffer));

    frame_pin (frame);
    *eax = *eax + file_read (f, page_buffer, read_bytes);
    frame_unpin(frame);

    remaining -= read_bytes;
    page_buffer = page_buffer + read_bytes;
    lock_release(&filesys_lock);
  }

  return;
}

static void
write (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];
  char *buffer = (char *)argv[1];
  unsigned size = (unsigned )argv[2];
  struct file *f;

  int remaining = size;
  unsigned page_buffer = buffer;
  unsigned nxt_boundary;

  int write_bytes;
  struct frame *frame;

  if (fd == 0) {
    abnormal_exit();
  }

  if (fd == 1) {
    putbuf(buffer, size);
    return;
  }

  f = thread_find_file(fd);

  if (!f) {
    abnormal_exit();
  }

  *eax = 0;
  while (remaining > 0) {
    if (check_uaddr (page_buffer)) {
      abnormal_exit();
    }

    lock_acquire(&filesys_lock);
    if (page_buffer % PGSIZE == 0) {
      write_bytes = remaining > PGSIZE ? PGSIZE : remaining;
    } else {
      nxt_boundary = pg_round_up (page_buffer);
      write_bytes = remaining > (nxt_boundary - page_buffer) ?
        (nxt_boundary - page_buffer) : remaining;
    }

    frame = frame_find (pagedir_get_page (thread_current()->pagedir, page_buffer));

    frame_pin (frame);
    *eax = *eax + file_write (f, page_buffer, write_bytes);
    frame_unpin(frame);

    remaining -= write_bytes;
    page_buffer = page_buffer + write_bytes;
    lock_release(&filesys_lock);
  }

  return;
}

static void
seek (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];
  unsigned pos = (unsigned) argv[1];

  struct file *f = thread_find_file(fd);

  lock_acquire(&filesys_lock);
  file_seek(f, pos);
  lock_release(&filesys_lock);
  return;
}

static void
tell (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];
  struct file *f = thread_find_file(fd);

  lock_acquire(&filesys_lock);
  *eax = file_tell (f);
  lock_release(&filesys_lock);
  return;
}

static void
close (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];
  struct file *f;

  if (fd < 2) {
    abnormal_exit();
  }

  f = thread_find_file(fd);

  if (!f) {
    abnormal_exit();
  }

  lock_acquire(&filesys_lock);
  file_close(f);
  lock_release(&filesys_lock);

  return;
}

static void
mmap (void **argv, uint32_t *eax, uint32_t *esp) {
  int fd = (int) argv[0];
  void *addr = (void *) argv[1];

  struct file *old_file, *f;
  int remaining, size;
  int read_bytes;
  off_t ofs;

  void *page_addr;
  uint32_t *pd = thread_current()->pagedir;

  if (fd < 2) {
    *eax = MAP_FAILED;
    return;
  }

  if (pg_round_down (addr) != addr || !addr) {
    *eax = MAP_FAILED;
    return;
  }

  old_file = thread_find_file (fd);
  if (old_file == NULL) {
    *eax = MAP_FAILED;
    return;
  }

  f = file_reopen (thread_find_file(fd));

  lock_acquire(&filesys_lock);
  size = file_length (f);
  remaining = size;

  if (remaining == 0) {
    file_close (f);
    lock_release(&filesys_lock);
    *eax = MAP_FAILED;
    return;
  }

  lock_release(&filesys_lock);

  for (ofs = 0; ofs < size; ofs += PGSIZE) {
    page_addr = (unsigned) addr + ofs;
    if (page_lookup (page_addr) != NULL) {
      *eax = MAP_FAILED;
      return;
    }

    read_bytes = remaining > PGSIZE ? PGSIZE : remaining;
    s_page_insert_file (page_addr, f, ofs, read_bytes, (PGSIZE - read_bytes), true);

    remaining -= PGSIZE;
  }

  mmap_add (addr, f);
  *eax = f->fd;

  return;
}

static void
munmap (void **argv, uint32_t *eax, uint32_t *esp) {
  mapid_t mapid = (mapid_t) argv[0];
  struct mmap_info *m;
  struct list_elem *curr;
  struct file *f;
  bool found = false;

  for (curr=list_begin(&mmap_list); curr!=list_tail(&mmap_list);
        curr=list_next(curr)) {
    m = list_entry (curr, struct mmap_info, elem);
    if ((m->file)->fd == mapid) {
      f = m->file;
      found = true;
      break;
    }
  }

  if (!found) {
    abnormal_exit ();
  }

  mmap_remove (m);
  list_remove (&m->elem);
  free (m);

  return;
}

