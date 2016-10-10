#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static void get_user (const uint8_t *uaddr, void *save_to, size_t size);
static void put_user (const uint8_t *uaddr, void *copy_from, size_t size);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  void *esp = f->esp;
  uint32_t *syscall_nr;

  get_user(esp, syscall_nr, 1);
  hex_dump(0, esp, PHYS_BASE-esp, true);
  printf ("%p %u\n", esp, *syscall_nr);

  printf ("system call!\n");
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
