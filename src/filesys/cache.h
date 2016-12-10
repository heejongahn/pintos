#include "threads/synch.h"
#include "devices/disk.h"

struct bcache_elem {
  uint8_t disk_sector_size[DISK_SECTOR_SIZE];

  bool valid;
  bool accessed;
  bool dirty;
};

void bcache_init (void);
