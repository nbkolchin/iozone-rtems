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
#include "msdos_format.h"
#include "iozone_cfg.h"

#ifndef COMBO_RTEMS
#include <rtems/bdpart.h>
#include <rtems/rtems-rfs-format.h>
#endif

#if 0
#undef FAT_BENCH
#else
#define FAT_BENCH
#endif

#if 1
#undef FORMAT_ENABLE
#else
#define FORMAT_ENABLE
#endif

#define DEVNAME "/dev/hda"

#ifdef __PPC__
static unsigned char _fpga_data[] =
{
#include "ppc-altera/TDS12_Base.ttf"
};
unsigned long fpga_data_len = sizeof(_fpga_data);
unsigned char* fpga_data = _fpga_data;
#endif

#ifdef COMBO_RTEMS
fstab_t fs_table = {
  "/dev/hda",
  "/mnt/flash",
  &msdos_ops,
  RTEMS_FILESYSTEM_READ_WRITE,
  FSMOUNT_MNT_OK|FSMOUNT_MNTPNT_CRTERR|FSMOUNT_MNT_FAILED,
  0
};
#else
static const rtems_fstab_entry fstab[] = {
#ifdef FAT_BENCH
  {
    .source = "/dev/hda",
    .target = "/mnt/flash",
    .type = "dosfs",
    .options = RTEMS_FILESYSTEM_READ_WRITE,
    .report_reasons = RTEMS_FSTAB_ANY,
    .abort_reasons = RTEMS_FSTAB_OK
  }
#else
  {
    .source = "/dev/hda",
    .target = "/mnt/flash",
    .type = "rfs",
    .options = RTEMS_FILESYSTEM_READ_WRITE,
    .report_reasons = RTEMS_FSTAB_ANY,
    .abort_reasons = RTEMS_FSTAB_NONE
  }
#endif
};
#endif

static int init_ide()
{
  int rc = 0;

  char buf[512];

#ifndef COMBO_RTEMS
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  size_t abort_index = 0;

#ifndef FAT_BENCH
  rtems_rfs_format_config rfs_config =
  {
     .block_size = 0,        /* The size of a block. */
     .group_blocks = 0,      /* The number of blocks in a group. */
     .group_inodes = 0,      /* The number of inodes in a group. */
     .inode_overhead = 0,    /* The percentage overhead allocated to inodes. */
     .max_name_length = 0,   /* The maximum length of a name. */
     .initialise_inodes = 0, /* Initialise the inode tables to all ones. */
     .verbose = 1            /* Is the format verbose.  */
  };
#endif
#endif

#ifdef COMBO_RTEMS
  printk(".. COMBO RTEMS ..\n");
#else
  printk(".. VANILLA RTEMS ..\n");
#endif

#ifdef FAT_BENCH
  printk(".. FAT benchmarking. Preparing/formatting partition\n");
#ifdef FORMAT_ENABLE
  rc = msdos_format("/dev/hda", NULL);
  if (rc != 0)
     printk(".. msdos_format(/dev/hda) failed: %s\n", strerror(errno));
  else
     printk(".. msdos_format is OK\n");
#endif
#else
  printk(".. RFS benchmarking. Preparing/formatting disk\n");
#if 0
  {
      int i;

#define PARTITION_COUNT 1

      rtems_bdpart_partition created_partitions[PARTITION_COUNT];

      static const rtems_bdpart_format format = {
         .mbr = {
            .type = RTEMS_BDPART_FORMAT_MBR,
            .disk_id = 0xdeadbeef,
            .dos_compatibility = false
         }
      };

      static const unsigned distribution[PARTITION_COUNT] = {
            1
      };

      memset(&created_partitions[0], 0, sizeof(created_partitions));

      for (i = 0; i < PARTITION_COUNT; ++i) {
          rtems_bdpart_to_partition_type(RTEMS_BDPART_MBR_EMPTY,
                                         created_partitions[i].type);
      }

      sc = rtems_bdpart_create("/dev/hda", &format,
                               &created_partitions[0],
                               &distribution[0],
                               PARTITION_COUNT);
      if (sc != 0)
          printk(".. rtems_bdpart_create(/dev/hda) failed: %s\n", strerror(errno));
      else
          printk(".. rtems_bdpart_create() is OK\n");

      sc = rtems_bdpart_write("/dev/hda", &format,
                              &created_partitions[0],
                              PARTITION_COUNT);
      if (sc != 0)
          printk(".. rtems_bdpart_write(/dev/hda) failed: %s\n", strerror(errno));
      else
          printk(".. rtems_bdpart_write() is OK\n");
  }
#endif

#ifdef FORMAT_ENABLE
  rc = rtems_rfs_format("/dev/hda", &rfs_config);
  if (rc != 0)
     printk(".. rfs_format(/dev/hda) failed: %s\n", strerror(errno));
  else
     printk(".. rfs_format is OK\n");
#endif

#endif /* FAT_BENCH */

  /* Create a mount point folder */
  rc = mkdir("/mnt", 0777);
  if(rc == -1)
  {
    printk("mkdir failed: %s\n", strerror(errno));
    exit(3);
  }

#ifdef COMBO_RTEMS
  /* Some checking needed for combo version only */
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
  /* end of checking */

#if 0
  rc = rtems_ide_part_table_initialize(DEVNAME);
  if(rc != RTEMS_SUCCESSFUL)
  {
    printk("ide_part_table failed: %i\n", rc);
    exit(4);
  }
#endif

  rc = rtems_fsmount(&fs_table, 1, NULL);
  if(rc != 1)
  {
    printk("rtems_fsmount failed: %i\n", rc);
    exit(5);
  }
#else

#if 0
  sc = rtems_bdpart_register_from_disk("/dev/hda");
  if (sc != RTEMS_SUCCESSFUL) {
    printk("read partition table failed: %s\n", rtems_status_text(sc));
    exit(4);
  }
  else
  {
    printk(".. partition table of \"/dev/hda\" is OK\n");
  }
#endif

  rc = rtems_fsmount(fstab, sizeof(fstab) / sizeof(fstab [0]), &abort_index);
  if (rc != 0) {
    printk("mount failed: %s\n", strerror(errno));
    exit(5);
  }
  else
  {
    printk(".. %s() returns OK\n", __FUNCTION__);
  }

  if (abort_index)
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
#ifdef COMBO_RTEMS
  rc = rtems_shell_init("SHll", 32000, 100, "/dev/tty1", 1, 0);
#else
  rc = rtems_shell_init("SHll", 32000, 100, "/dev/tty1", 1, 0, NULL);
#endif
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
