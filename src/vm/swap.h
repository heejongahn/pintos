#include <bitmap.h>
#include "vm/page.h"
#include "devices/disk.h"

static struct disk *swap_disk;
static struct bitmap *swap_table;

void swap_init (void);

bool swap_in (struct s_page *);

/*
bool swap_out (uint8_t *);
bool swap_find (
*/
