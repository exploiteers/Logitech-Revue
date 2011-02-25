/*
 * Copyright 2006 (c) Texas Instruments
 * Copyright 2008 (c) MontaVista Software
 *
 * Header file for DaVinci EMAC platform data
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#ifndef __DAVINCI_EMAC_PLATFORM_DATA
#define __DAVINCI_EMAC_PLATFORM_DATA

/**
 *  EMAC RX configuration
 *
 *  This data structure configures the RX module of the device
 */
struct emac_rx_config {
	bool pass_crc;		/* pass CRC bytes to packet memory */
	bool qos_enable;	/* receive qo_s enable ? */
	bool no_buffer_chaining;	/* DEBUG ONLY - ALWAYS SET TO FALSE */
	bool copy_maccontrol_frames_enable;
	bool copy_short_frames_enable;
	bool copy_error_frames_enable;
	bool promiscous_enable;
	u32 promiscous_channel;	/* promiscous receive channel */
	bool broadcast_enable;	/* receive broadcast frames ? */
	u32 broadcast_channel;	/* broadcast receive channel */
	bool multicast_enable;	/* receive multicast frames ? */
	u32 multicast_channel;	/* multicast receive channel */
	u32 max_rx_pkt_length;	/* max receive packet length */
	u32 buffer_offset;	/* buffer offset for all RX channels */
};

/**
 *  EMAC Init Configuration
 *
 *  Configuration information provided to DDC layer during
 *  initialization.  DDA gets the config information from the OS/PAL
 *  layer and passes the relevant config to the DDC during
 *  initialization. The config info can come from various sources -
 *  static compiled in info, boot time (ENV, Flash) info etc.
 */
struct emac_init_config {
	u32 num_tx_channels;
	u32 num_rx_channels;
	u32 emac_bus_frequency;
	u32 reset_line;
	u32 mdio_reset_line;
	u32 mdio_intr_line;
	u32 m_link_mask;
	u32 mdio_bus_frequency;
	u32 mdio_clock_frequency;
	u32 mdio_tick_msec;
	u32 mib64cnt_msec;
	u32 phy_mode;
	u32 registers_old;
	u32 gigabit;
	struct emac_rx_config rx_cfg;

	int mdio_irqs[32];
};

#endif

