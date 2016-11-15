#include <bitmap.h>
#include "vm/page.h"
#include "devices/disk.h"

static struct bitmap *swap_table;

void swap_init (void);

/*
bool swap_in (uint8_t *);
bool swap_out (uint8_t *);
bool swap_find (
*/
