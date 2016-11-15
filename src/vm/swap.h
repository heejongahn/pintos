#include <bitmap.h>
#include "devices/disk.h"

static struct disk *swap_disk;
static struct bitmap *swap_table;

void swap_init (void);

bool swap_in (size_t, uint8_t *);
bool swap_out (struct s_page *);

/*
bool swap_find (
*/
