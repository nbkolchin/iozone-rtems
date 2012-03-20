#ifndef __SNOB_RTEMS_TELNET_COMPAT_LAYER_H__
#define __SNOB_RTEMS_TELNET_COMPAT_LAYER_H__ 1

#include <rtems.h>

#include <rtems/telnetd.h>

#if (__RTEMS_MAJOR__ >= 4) && (__RTEMS_MINOR__ >= 8) && (__RTEMS_MINOR__ < 10)

#include <rtems/shell.h>
#include <string.h>

static void __snob_static_telnet_cmd(char* a, void* v)
{
  extern rtems_shell_env_t* rtems_shell_init_env(rtems_shell_env_t*);
  rtems_shell_env_t* sea;
  sea = rtems_shell_init_env(NULL);
  sea->devname = a;
  sea->taskname = 0;
  sea->exit_shell = 0;
  sea->forever = 0;
  sea->input = strdup("stdin");
  sea->output = strdup("stdout");
  sea->output_append = 0;
  sea->wake_on_end = RTEMS_INVALID_ID;
  rtems_shell_main_loop(sea);
}

/* wrapper around new function */
#define rtems_telnetd_initialize() \
  rtems_telnetd_initialize( \
      __snob_static_telnet_cmd, \
      0, \
      0, \
      0, \
      100, \
      0);

/* ignores arguments */
#define rtems_telnetd_main(a, b) rtems_telnetd_initialize()

/* noop */
#define rtems_telnetd_register() (1)

#define rtems_initialize_telnetd        rtems_telnetd_initialize
#define main_telnetd                    rtems_telnetd_main
#define register_telnetd                rtems_telnetd_register

#endif

#if (__RTEMS_MAJOR__ >= 4) && (__RTEMS_MINOR__ >= 10)
rtems_telnetd_config_table rtems_telnetd_config;
#endif

#endif
