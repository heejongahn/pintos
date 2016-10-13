#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
static void get_user (const uint8_t *uaddr, void *save_to, size_t size);
static void put_user (const uint8_t *uaddr, void *copy_from, size_t size);

typedef void (*handler) (void *, uint32_t *);

static void halt (void **argv, uint32_t *eax);
static void exit (void **argv, uint32_t *eax);
static void exec (void **argv, uint32_t *eax);
static void wait (void **argv, uint32_t *eax);
static void create (void **argv, uint32_t *eax);
static void remove (void **argv, uint32_t *eax);
static void open (void **argv, uint32_t *eax);
static void filesize (void **argv, uint32_t *eax);
static void read (void **argv, uint32_t *eax);
static void write (void **argv, uint32_t *eax);
static void seek (void **argv, uint32_t *eax);
static void tell (void **argv, uint32_t *eax);
static void close (void **argv, uint32_t *eax);

static handler handlers[13] = {
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
  &close
};

static int *abnormal_exit_argv[1] = {-1};

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t *esp = f->esp;
  uint32_t *eax = &f->eax;
  uint32_t syscall_nr;
  int argc, i;
  void *argv[3];

  memset (argv, 0, sizeof (void *) * 4);
  get_user(esp, &syscall_nr, 4);
  // printf("System call number: %d\n", syscall_nr);
  // printf("Current esp: %p\n", esp);

  /*
  while (esp < PHYS_BASE) {
    printf("%x\n", *esp);
  esp++;
  }
  */

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
      argc = 1;
      break;
    case SYS_CREATE:
    case SYS_SEEK:
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

  // printf("System call argc: %d\n", argc);

  for (i=0; i<argc; i++) {
    get_user((esp+i), &(argv[i]), 4);
    // printf("System call argument #%d(at %p): %p\n", i, (esp+i), (argv[i]));
  }

  // hex_dump(0, esp, PHYS_BASE - (unsigned) esp, true);
  (*handlers[syscall_nr]) (argv, eax);

  return;
}

/* Reads SIZE bytes at user virtual address UADDR.
   Power off if failed. */
static void
get_user (const uint8_t *uaddr, void *save_to, size_t size)
{
  uint8_t *check = uaddr;
  for (check; check < uaddr + size; check++) {
    if ((!check) || is_kernel_vaddr(check)) {
      return exit(abnormal_exit_argv, NULL);
    }

    if (!pagedir_get_page(thread_current()->pagedir, uaddr)) {
      return exit(abnormal_exit_argv, NULL);
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
    if ((!check) || is_kernel_vaddr(check)) {
      return exit(abnormal_exit_argv, NULL);
    }

    if (!pagedir_get_page(thread_current()->pagedir, uaddr)) {
      return exit(abnormal_exit_argv, NULL);
    }
  }


  memcpy (uaddr, copy_from, size);
}


static void
halt (void **argv, uint32_t *eax) {
  power_off();
  return;
}

static void
exit (void **argv, uint32_t *eax) {
  struct thread *t = thread_current ();
  t->exit_code = (int) argv[0];
  printf("%s: exit(%d)\n",
      thread_current()->name, thread_current()->exit_code);
  sema_up(&t->exiting);

  thread_exit();
  return;
}

static void
exec (void **argv, uint32_t *eax) {
  return;
}

static void
wait (void **argv, uint32_t *eax) {
  return;
}

static void
create (void **argv, uint32_t *eax) {
  char *file = (char *) argv[0];
  unsigned initial_size = (unsigned) argv[1];


  if ((!file) || is_kernel_vaddr(file)) {
    return exit(abnormal_exit_argv, eax);
  }

  if (!pagedir_get_page(thread_current()->pagedir, file)) {
    return exit(abnormal_exit_argv, eax);
  }

  lock_acquire (&filesys_lock);
  *eax = filesys_create(file, initial_size);
  lock_release (&filesys_lock);
  return;
}

static void
remove (void **argv, uint32_t *eax) {
  char *name = (char *) argv[0];

  if ((!name) || is_kernel_vaddr(name)) {
    return exit(abnormal_exit_argv, eax);
  }

  if (!pagedir_get_page(thread_current()->pagedir, name)) {
    return exit(abnormal_exit_argv, eax);
  }

  lock_acquire (&filesys_lock);
  *eax = filesys_remove(name);
  lock_release (&filesys_lock);
  return;
}

static void
open (void **argv, uint32_t *eax) {
  return;
}

static void
filesize (void **argv, uint32_t *eax) {
  return;
}

static void
read (void **argv, uint32_t *eax) {
  return;
}

static void
write (void **argv, uint32_t *eax) {
  int fd = (int) argv[0];
  char *buf = (char *)argv[1];
  unsigned size = (unsigned )argv[2];

  if (fd == 1) {
    putbuf(buf, size);
  }

  *eax = size;

  return;
}

static void
seek (void **argv, uint32_t *eax) {
  return;
}

static void
tell (void **argv, uint32_t *eax) {
  return;
}

static void
close (void **argv, uint32_t *eax) {
  return;
}
