#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

static void syscall_handler (struct intr_frame *);
static void get_user (const uint8_t *uaddr, void *save_to, size_t size);
static void put_user (const uint8_t *uaddr, void *copy_from, size_t size);

typedef void (*handler) (void *);

static void halt (void *argv);
static void exit (void *argv);
static void exec (void *argv);
static void wait (void *argv);
static void create (void *argv);
static void remove (void *argv);
static void open (void *argv);
static void filesize (void *argv);
static void read (void *argv);
static void write (void *argv);
static void seek (void *argv);
static void tell (void *argv);
static void close (void *argv);

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

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  uint32_t *esp = f->esp;
  uint32_t *syscall_nr;
  int argc, i;
  void **argv;

  get_user(esp, syscall_nr, 4);
  printf("System call number: %d\n", *syscall_nr);
  switch (*syscall_nr) {
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

  printf("System call argc: %d\n", argc);

  argv = malloc (argc * 4);
  memset (argv, 0, argc * 4);

  for (i=0; i<argc; i++) {
    get_user((esp + i + 1), &argv[i], 4);
    printf("System call argument #%d: %x\n", i, argv[i]);
  }

  printf ("system call!\n");
  free (argv);
  thread_exit ();
}

// TODO: Unmapped virtual address check
/* Reads SIZE bytes at user virtual address UADDR.
   Power off if failed. */
static void
get_user (const uint8_t *uaddr, void *save_to, size_t size)
{
  uint8_t *check = uaddr;
  for (check; check < uaddr + size; check++) {
    if ((!check) || is_kernel_vaddr(check))
      power_off();
  }

  memcpy (save_to, uaddr, size);
}

// TODO: Unmapped virtual address check
/* Writes SIZE bytes to user address UADDR, copying from COPY_FROM.
   Power off if failed. */
static void
put_user (const uint8_t *uaddr, void *copy_from, size_t size)
{
  uint8_t *check = uaddr;
  for (check; check < uaddr + size; check++) {
    if ((!check) || is_kernel_vaddr(check))
      power_off();
  }

  memcpy (uaddr, copy_from, size);
}


static void
halt (void *argv) {
  return;
}

static void
exit (void *argv) {
  return;
}

static void
exec (void *argv) {
  return;
}

static void
wait (void *argv) {
  return;
}

static void
create (void *argv) {
  return;
}

static void
remove (void *argv) {
  return;
}

static void
open (void *argv) {
  return;
}

static void
filesize (void *argv) {
  return;
}

static void
read (void *argv) {
  return;
}

static void
write (void *argv) {
  return;
}

static void
seek (void *argv) {
  return;
}

static void
tell (void *argv) {
  return;
}

static void
close (void *argv) {
  return;
}
