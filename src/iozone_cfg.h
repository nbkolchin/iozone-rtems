#ifndef __IOZONE_CFG_H__
#define __IOZONE_CFG_H__

/* The next functionalilties are disabled as a result of rtems port. 
 * Some of them are not supported by rtems and other ones are excluded
 * because of platform  limitations or provided functionality is
 * optional for current purposes. So some of them can be enabled
 * and debugged in some future on special request.
 */

/* #define NO_THREADS */
#undef PIT_ENABLED
#undef NET_BENCH
#undef ASYNC_IO
#undef SHARED_MEM
#undef MIX_PERF_TEST
#undef EXCEL
#undef IMON_ENABLED
#undef SIGNAL_ENABLE
#undef POPEN_PCLOSE_ENABLED
#undef LRAND
#undef SYSTEM_MOUNT

int main_iozone(int argc, char **argv);

#endif /* __IOZONE_CFG_H__ */
