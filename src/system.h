/*  system.h
 *
 *  This include file contains information that is included in every
 *  function in the test set.
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  system.h,v 1.13.6.1 2003/09/04 18:46:30 joel Exp
 */

#include <rtems.h>

/* functions */

rtems_task Init(
  rtems_task_argument argument
);

/* configuration information */

#include <bsp.h> /* for device driver prototypes */
#include <libchip/ata.h> /* for ata driver prototype */
#include <libchip/ide_ctrl.h> /* for general ide driver prototype */

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#if 0
#define CONFIGURE_HAS_OWN_DEVICE_DRIVER_TABLE

#define NULL_DRIVER_TABLE_ENTRY \
 { NULL, NULL, NULL, NULL, NULL, NULL }

rtems_driver_address_table Device_drivers[] =
	{
	CONSOLE_DRIVER_TABLE_ENTRY
	,CLOCK_DRIVER_TABLE_ENTRY
#ifdef RTEMS_BSP_HAS_IDE_DRIVER
	,IDE_CONTROLLER_DRIVER_TABLE_ENTRY
	/* important: ATA driver must be after ide drivers */
	,ATA_DRIVER_TABLE_ENTRY 
#endif
	, NULL_DRIVER_TABLE_ENTRY
	};
#endif

#define CONFIGURE_MAXIMUM_PARTITIONS              16
#define CONFIGURE_MAXIMUM_TASKS                   8
#define CONFIGURE_MAXIMUM_TIMERS                  8
#define CONFIGURE_MAXIMUM_SEMAPHORES              16
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES          16
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS  20

#define CONFIGURE_MICROSECONDS_PER_TICK 10000

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_STACK_SIZE  32000

#define CONFIGURE_EXTRA_TASK_STACKS         (64 * 64 * RTEMS_MINIMUM_STACK_SIZE)

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_FILESYSTEM_RFS
#define CONFIGURE_FILESYSTEM_DOSFS

/* #define CONFIGURE_BDBUF_BUFFER_COUNT 8 */

#include <rtems/confdefs.h>

/* end of include file */
