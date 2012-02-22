#define CONFIGURE_INIT
#include "system.h"

#include <rtems.h>
#include <rtems/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <rtems/dosfs.h>
#include <rtems/fsmount.h>
#include <rtems/ide_part_table.h>

#define DEVNAME "/dev/hda"

fstab_t fs_table = {
  "/dev/hda1",
  "/mnt/flash",
  &msdos_ops,
  RTEMS_FILESYSTEM_READ_WRITE,
  FSMOUNT_MNT_OK|FSMOUNT_MNTPNT_CRTERR|FSMOUNT_MNT_FAILED,
  0
};

static int init_ide()
{
  int rc;
  char buf[512];
  rc = open(DEVNAME, O_RDWR);
  if(rc == -1)
  {
    printk("dev open failed: %s\n", strerror(errno));
    exit(1);
  }
  if(read(rc, buf, 512) != 512)
  {
    printk("dev read failed: %s\n", strerror(errno));
    exit(2);
  }
  close(rc);
  rc = mkdir("/mnt", 0777);
  if(rc == -1)
  {
    printk("mkdir failed: %s\n", strerror(errno));
    exit(3);
  }
  rc = rtems_ide_part_table_initialize(DEVNAME);
  if(rc != RTEMS_SUCCESSFUL)
  {
    printk("ide_part_table failed: %i\n", rc);
    exit(4);
  }
  rc = rtems_fsmount(&fs_table, 1, NULL);
  if(rc != 1)
  {
    printk("rtems_fsmount failed: %i\n", rc);
    exit(5);
  }
  return 0;
}

rtems_task Init(rtems_task_argument unused)
{
  int rc;
  struct stat st;

  rc = init_ide();
  if(rc)
  {
    printk("init_ide failed: %i\n", rc);
    exit(2);
  }

  exit(0);
}  


