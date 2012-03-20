#pragma once

#if defined(__i386__)
#include "net-cfg-qemu.h"
#elif defined(__PPC__)
#include "net-cfg-ppc.h"
#elif defined(__SH4__)
#include "net-cfg-sh4.h"
#endif

#ifndef NET_CFG_IP_MASK
#define NET_CFG_IP_MASK "255.255.255.0"
#endif

#ifndef NET_CFG_NAME_SERVER
#define NET_CFG_NAME_SERVER { NET_CFG_GATEWAY }
#endif

#ifndef NET_CFG_LOG_HOST
#define NET_CFG_LOG_HOST NULL
#endif

#ifndef NET_CFG_NTP_SERVER
#define NET_CFG_NTP_SERVER { NULL }
#endif

#if RTEMS_USE_LWIPNET
#include <rtems/rtems_lwipnet.h>
#define rtems_bsdnet_ifconfig rtems_lwipnet_ifconfig
#define rtems_bsdnet_config rtems_lwipnet_config
#define rtems_loopattach rtems_lwipnet_loopattach
#define rtems_initialize_network rtems_lwipnet_initialize_network
extern int rtems_lwipnet_loopattach(struct rtems_lwipnet_ifconfig*, int);
#define NET_CFG_MBUF_CAPACITY 0
#define NET_CFG_MBUF_CLUSTER 0
#else
#include <rtems/rtems_bsdnet.h>
extern int rtems_bsdnet_loopattach(struct rtems_bsdnet_ifconfig*, int);
#define rtems_loopattach rtems_bsdnet_loopattach
#define rtems_initialize_network rtems_bsdnet_initialize_network
#define NET_CFG_MBUF_CAPACITY (1024*1024)
#define NET_CFG_MBUF_CLUSTER (1024*1024)
#endif

#if __RTEMS_MAJOR__ == 4 && __RTEMS_MINOR__ < 10
static struct rtems_bsdnet_ifconfig loopback_config = {
  "lo0",            /* name */
  rtems_loopattach, /* attach function */
  NULL,             /* link to next interface */
  "127.0.0.1",      /* ip address */
  "255.0.0.0",      /* ip net mask */
};
#endif

static char net_cfg_ethernet_address[6] = NET_CFG_ETHERNET_ADDRESS;

static struct rtems_bsdnet_ifconfig netdriver_config = {
  RTEMS_BSP_NETWORK_DRIVER_NAME,   /* name */
  RTEMS_BSP_NETWORK_DRIVER_ATTACH, /* attach function */
#if __RTEMS_MAJOR__ == 4 && __RTEMS_MINOR__ < 10
  &loopback_config,                /* link to next interface */
#else
  NULL,
#endif
  NET_CFG_IP_ADDRESS,              /* ip address */
  NET_CFG_IP_MASK,                 /* ip net mask */
  net_cfg_ethernet_address,        /* MAC address */
};

struct rtems_bsdnet_config rtems_bsdnet_config = {
  &netdriver_config,     /* network interface */
  NULL,                  /* fixed network configuration */
  5,                     /* default network task prioirty */
  NET_CFG_MBUF_CAPACITY, /* default MBUF capacity */
  NET_CFG_MBUF_CLUSTER,  /* default MBUF cluster capacity */
  "rtems",               /* hostname */
  "oktetlabs.com",       /* domain name */
  NET_CFG_GATEWAY,       /* gateway */
  NET_CFG_LOG_HOST,      /* log host */
  NET_CFG_NAME_SERVER,   /* Name server[s] */
  NET_CFG_NTP_SERVER,    /* NTP server[s] */
};
