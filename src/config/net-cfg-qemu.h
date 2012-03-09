#define NET_CFG_ETHERNET_ADDRESS { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 }
#define NET_CFG_IP_ADDRESS "192.168.254.1"
#define NET_CFG_GATEWAY "192.168.254.254"

#if 0
char ethernet_address [6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
// char ip_address [20] = {'1','0','.','1','0','0','.','7','.','1','0',0};
#endif

#if 0
static struct rtems_bsdnet_ifconfig netdriver_config = {
	RTEMS_BSP_NETWORK_DRIVER_NAME,		/* name */
	RTEMS_BSP_NETWORK_DRIVER_ATTACH,	/* attach function */

	&loopback_config,		/* link to next interface */

	"192.168.254.1",		/* IP address */
 	"255.255.255.0",		/* IP net mask */
	ethernet_address,               /* Ethernet hardware address */
	0,
	0,
	0,
	0,
	0,
	9
};
#endif
