#ifndef _LTT_FACILITY_LOADER_NETWORK_IP_INTERFACE_H_
#define _LTT_FACILITY_LOADER_NETWORK_IP_INTERFACE_H_

#ifdef CONFIG_LTT

#include <linux/ltt-facilities.h>
#include <ltt/ltt-facility-id-network_ip_interface.h>

ltt_facility_t	ltt_facility_network_ip_interface;
ltt_facility_t	ltt_facility_network_ip_interface_7A3120EF;

#define LTT_FACILITY_SYMBOL		ltt_facility_network_ip_interface
#define LTT_FACILITY_CHECKSUM_SYMBOL	ltt_facility_network_ip_interface_7A3120EF
#define LTT_FACILITY_CHECKSUM		0x7A3120EF
#define LTT_FACILITY_NAME		"network_ip_interface"
#define LTT_FACILITY_NUM_EVENTS	facility_network_ip_interface_num_events

#endif //CONFIG_LTT

#endif //_LTT_FACILITY_LOADER_NETWORK_IP_INTERFACE_H_
