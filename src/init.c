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

#include <rtems/shell.h>

#include "config/net-cfg.h"
#include "telnet_compat.h"
#include "iozone_cfg.h"

#if 1
#define COMBO_RTEMS
#else
#undef COMBO_RTEMS
#endif

#ifndef COMBO_RTEMS
#include <rtems/bdpart.h>
#endif

#define DEVNAME "/dev/hda"


#if 0
static const rtems_fstab_entry fstab [] = {
  {
    .source = "/dev/sd-card-a",
    .target = "/mnt",
    .type = "dosfs",
    .options = RTEMS_FILESYSTEM_READ_WRITE,
    .report_reasons = RTEMS_FSTAB_ANY,
    .abort_reasons = RTEMS_FSTAB_OK
  }, {
    .source = "/dev/sd-card-a1",
    .target = "/mnt",
    .type = "dosfs",
    .options = RTEMS_FILESYSTEM_READ_WRITE,
    .report_reasons = RTEMS_FSTAB_ANY,
    .abort_reasons = RTEMS_FSTAB_NONE
  }
};

static void my_mount(void)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  int rv = 0;
  size_t abort_index = 0;

  sc = rtems_bdpart_register_from_disk("/dev/sd-card-a");
  if (sc != RTEMS_SUCCESSFUL) {
    printf("read partition table failed: %s\n", rtems_status_text(sc));
  }

  rv = rtems_fsmount(fstab, sizeof(fstab) / sizeof(fstab [0]), &abort_index);
  if (rv != 0) {
    printf("mount failed: %s\n", strerror(errno));
  }
  printf("mount aborted at %zu\n", abort_index);
}

#endif

#ifdef COMBO_RTEMS
fstab_t fs_table = {
  "/dev/hda1",
  "/mnt/flash",
  &msdos_ops,
  RTEMS_FILESYSTEM_READ_WRITE,
  FSMOUNT_MNT_OK|FSMOUNT_MNTPNT_CRTERR|FSMOUNT_MNT_FAILED,
  0
};
#else
static const rtems_fstab_entry fstab[] = {
  {
    .source = "/dev/hda",
    .target = "/mnt/flash",
    .type = "dosfs",
    .options = RTEMS_FILESYSTEM_READ_WRITE,
    .report_reasons = RTEMS_FSTAB_ANY,
    .abort_reasons = RTEMS_FSTAB_OK
  }, {
    .source = "/dev/hda1",
    .target = "/mnt",
    .type = "dosfs",
    .options = RTEMS_FILESYSTEM_READ_WRITE,
    .report_reasons = RTEMS_FSTAB_ANY,
    .abort_reasons = RTEMS_FSTAB_NONE
  }
};
#endif

static int init_ide()
{
  int rc;
  char buf[512];

#ifndef COMBO_RTEMS
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  int rv = 0;
  size_t abort_index = 0;
#endif

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
#ifdef COMBO_RTEMS
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
#else
  sc = rtems_bdpart_register_from_disk("/dev/hda");
  if (sc != RTEMS_SUCCESSFUL) {
    printf("read partition table failed: %s\n", rtems_status_text(sc));
    exit(4);
  }

  rv = rtems_fsmount(fstab, sizeof(fstab) / sizeof(fstab [0]), &abort_index);
  if (rv != 0) {
    printf("mount failed: %s\n", strerror(errno));
    exit(5);
  }

  printf("mount aborted at %zu\n", abort_index);
#endif

  printf(".. CompactFlash logical disk has been mounted\n");
  return 0;
}

static int iozone_func(int argc, char** argv)
{
  return main_iozone(argc, argv);
}

static rtems_shell_cmd_t iozone_cmd = {
  "iozone",         /* name */
  "execute iozone", /* help text */
  "misc",           /* topic */
  iozone_func,      /* function */
  NULL,             /* alias */
  NULL              /* next */
};

/* initialize shell, network and telnet */
static void init_telnetd()
{
  rtems_status_code rc;
  rtems_termios_initialize();
  rtems_initialize_network();
  rtems_telnetd_initialize();
  rc = rtems_shell_init("SHll", 32000, 100, "/dev/tty1", 1, 0);
  if(rc != RTEMS_SUCCESSFUL)
    printk("init shell on console failed\n");
}

rtems_task Init(rtems_task_argument unused)
{
  init_ide();
  init_telnetd();
  if(rtems_shell_add_cmd_struct(&iozone_cmd) == NULL)
  {
    printk("add iozone_cmd to shell failed");
    exit(3);
  }

  rtems_task_delete(RTEMS_SELF);

  exit(0);
}  


