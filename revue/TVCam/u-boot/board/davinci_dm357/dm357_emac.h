/*
 * dm644x_emac.h
 *
 * TI DaVinci (DM644X) EMAC peripheral driver header for DV-EVM
 *
 * Copyright (C) 2005 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 
 * Modifications:
 * ver. 1.0: Sep 2005, TI PSP Team - Created EMAC version for uBoot.
 *
 */
 
#ifndef _DM644X_EMAC_H_
#define _DM644X_EMAC_H_

/***********************************************
 ********** Configurable items *****************
 ***********************************************/
 
/* Addresses of EMAC module in DaVinci */
#define EMAC_BASE_ADDR             (0x01C80000)
#define EMAC_WRAPPER_BASE_ADDR     (0x01C81000)
#define EMAC_WRAPPER_RAM_ADDR      (0x01C82000)
#define EMAC_MDIO_BASE_ADDR        (0x01C84000)

/* MDIO module input frequency */
#define EMAC_MDIO_BUS_FREQ          76500000    /* PLL/6 - 76.5 MHz */
/* MDIO clock output frequency */
#define EMAC_MDIO_CLOCK_FREQ        2200000     /* 2.2 MHz */

/* PHY mask - set only those phy number bits where phy is/can be connected */
#define EMAC_MDIO_PHY_MASK          0xFFFFFFFF

/* Ethernet Min/Max packet size */
#define EMAC_MIN_ETHERNET_PKT_SIZE  60
#define EMAC_MAX_ETHERNET_PKT_SIZE  1518
#define EMAC_PKT_ALIGN              18  /* 1518 + 18 = 1536 (packet aligned on 32 byte boundry) */

/* Number of RX packet buffers
 * NOTE: Only 1 buffer supported as of now 
 */
#define EMAC_MAX_RX_BUFFERS         10

/***********************************************
 ******** Internally used macros ***************
 ***********************************************/

#define EMAC_CH_TX                  1 
#define EMAC_CH_RX                  0

/* Each descriptor occupies 4, lets start RX desc's at 0 and 
 * reserve space for 64 descriptors max
 */
#define EMAC_RX_DESC_BASE           0x0
#define EMAC_TX_DESC_BASE           0x1000

/* EMAC Teardown value */
#define EMAC_TEARDOWN_VALUE         0xFFFFFFFC

/* MII Status Register */
#define MII_STATUS_REG              1

/* Intel LXT971 Digtal Config Register */
#define MII_DIGITAL_CONFIG_REG      26

/* Number of statistics registers */
#define EMAC_NUM_STATS              36

/* EMAC Descriptor */
typedef volatile struct _emac_desc 
{
  unsigned int      next;           /* Pointer to next descriptor in chain */
  unsigned char     *buffer;        /* Pointer to data buffer              */
  unsigned int      buff_off_len;   /* Buffer Offset(MSW) and Length(LSW)  */
  unsigned int      pkt_flag_len;   /* Packet Flags(MSW) and Length(LSW)   */
} emac_desc;

/* CPPI bit positions */
#define EMAC_CPPI_SOP_BIT               (0x80000000)  /*(1 << 31)*/
#define EMAC_CPPI_EOP_BIT               (0x40000000)  /*(1 << 30*/
#define EMAC_CPPI_OWNERSHIP_BIT         (0x20000000)  /*(1 << 29)*/
#define EMAC_CPPI_EOQ_BIT               (0x10000000)  /*(1 << 28)*/
#define EMAC_CPPI_TEARDOWN_COMPLETE_BIT (0x08000000)  /*(1 << 27)*/
#define EMAC_CPPI_PASS_CRC_BIT          (0x04000000)  /*(1 << 26)*/

#define EMAC_CPPI_RX_ERROR_FRAME        (0x03FC0000)

#define EMAC_MACCONTROL_MIIEN_ENABLE        (0x20)
#define EMAC_MACCONTROL_FULLDUPLEX_ENABLE   (0x1)

#define EMAC_RXMBPENABLE_RXCAFEN_ENABLE     (0x200000)
#define EMAC_RXMBPENABLE_RXBROADEN          (0x2000)


#define MDIO_CONTROL_ENABLE             (0x40000000)
#define MDIO_CONTROL_FAULT              (0x80000)
#define MDIO_USERACCESS0_GO             (0x80000000)
#define MDIO_USERACCESS0_WRITE_READ     (0x0)
#define MDIO_USERACCESS0_WRITE_WRITE    (0x40000000)



/* EMAC Register overlay */

/* Ethernet MAC Register Overlay Structure */
typedef volatile struct  {
    unsigned int TXIDVER;
    unsigned int TXCONTROL;
    unsigned int TXTEARDOWN;
    unsigned char RSVD0[4];
    unsigned int RXIDVER;
    unsigned int RXCONTROL;
    unsigned int RXTEARDOWN;
    unsigned char RSVD1[100];
    unsigned int TXINTSTATRAW;
    unsigned int TXINTSTATMASKED;
    unsigned int TXINTMASKSET;
    unsigned int TXINTMASKCLEAR;
    unsigned int MACINVECTOR;
    unsigned char RSVD2[12];
    unsigned int RXINTSTATRAW;
    unsigned int RXINTSTATMASKED;
    unsigned int RXINTMASKSET;
    unsigned int RXINTMASKCLEAR;
    unsigned int MACINTSTATRAW;
    unsigned int MACINTSTATMASKED;
    unsigned int MACINTMASKSET;
    unsigned int MACINTMASKCLEAR;
    unsigned char RSVD3[64];
    unsigned int RXMBPENABLE;
    unsigned int RXUNICASTSET;
    unsigned int RXUNICASTCLEAR;
    unsigned int RXMAXLEN;
    unsigned int RXBUFFEROFFSET;
    unsigned int RXFILTERLOWTHRESH;
    unsigned char RSVD4[8];
    unsigned int RX0FLOWTHRESH;
    unsigned int RX1FLOWTHRESH;
    unsigned int RX2FLOWTHRESH;
    unsigned int RX3FLOWTHRESH;
    unsigned int RX4FLOWTHRESH;
    unsigned int RX5FLOWTHRESH;
    unsigned int RX6FLOWTHRESH;
    unsigned int RX7FLOWTHRESH;
    unsigned int RX0FREEBUFFER;
    unsigned int RX1FREEBUFFER;
    unsigned int RX2FREEBUFFER;
    unsigned int RX3FREEBUFFER;
    unsigned int RX4FREEBUFFER;
    unsigned int RX5FREEBUFFER;
    unsigned int RX6FREEBUFFER;
    unsigned int RX7FREEBUFFER;
    unsigned int MACCONTROL;
    unsigned int MACSTATUS;
    unsigned int EMCONTROL;
    unsigned int FIFOCONTROL;
    unsigned int MACCONFIG;
    unsigned int SOFTRESET;
    unsigned char RSVD5[88];
    unsigned int MACSRCADDRLO;
    unsigned int MACSRCADDRHI;
    unsigned int MACHASH1;
    unsigned int MACHASH2;
    unsigned int BOFFTEST;
    unsigned int TPACETEST;
    unsigned int RXPAUSE;
    unsigned int TXPAUSE;
    unsigned char RSVD6[16];
    unsigned int RXGOODFRAMES;
    unsigned int RXBCASTFRAMES;
    unsigned int RXMCASTFRAMES;
    unsigned int RXPAUSEFRAMES;
    unsigned int RXCRCERRORS;
    unsigned int RXALIGNCODEERRORS;
    unsigned int RXOVERSIZED;
    unsigned int RXJABBER;
    unsigned int RXUNDERSIZED;
    unsigned int RXFRAGMENTS;
    unsigned int RXFILTERED;
    unsigned int RXQOSFILTERED;
    unsigned int RXOCTETS;
    unsigned int TXGOODFRAMES;
    unsigned int TXBCASTFRAMES;
    unsigned int TXMCASTFRAMES;
    unsigned int TXPAUSEFRAMES;
    unsigned int TXDEFERRED;
    unsigned int TXCOLLISION;
    unsigned int TXSINGLECOLL;
    unsigned int TXMULTICOLL;
    unsigned int TXEXCESSIVECOLL;
    unsigned int TXLATECOLL;
    unsigned int TXUNDERRUN;
    unsigned int TXCARRIERSENSE;
    unsigned int TXOCTETS;
    unsigned int FRAME64;
    unsigned int FRAME65T127;
    unsigned int FRAME128T255;
    unsigned int FRAME256T511;
    unsigned int FRAME512T1023;
    unsigned int FRAME1024TUP;
    unsigned int NETOCTETS;
    unsigned int RXSOFOVERRUNS;
    unsigned int RXMOFOVERRUNS;
    unsigned int RXDMAOVERRUNS;
    unsigned char RSVD7[624];
    unsigned int MACADDRLO;
    unsigned int MACADDRHI;
    unsigned int MACINDEX;
    unsigned char RSVD8[244];
    unsigned int TX0HDP;
    unsigned int TX1HDP;
    unsigned int TX2HDP;
    unsigned int TX3HDP;
    unsigned int TX4HDP;
    unsigned int TX5HDP;
    unsigned int TX6HDP;
    unsigned int TX7HDP;
    unsigned int RX0HDP;
    unsigned int RX1HDP;
    unsigned int RX2HDP;
    unsigned int RX3HDP;
    unsigned int RX4HDP;
    unsigned int RX5HDP;
    unsigned int RX6HDP;
    unsigned int RX7HDP;
    unsigned int TX0CP;
    unsigned int TX1CP;
    unsigned int TX2CP;
    unsigned int TX3CP;
    unsigned int TX4CP;
    unsigned int TX5CP;
    unsigned int TX6CP;
    unsigned int TX7CP;
    unsigned int RX0CP;
    unsigned int RX1CP;
    unsigned int RX2CP;
    unsigned int RX3CP;
    unsigned int RX4CP;
    unsigned int RX5CP;
    unsigned int RX6CP;
    unsigned int RX7CP;
} emac_regs;

/* EMAC Wrapper Register Overlay */
typedef volatile struct  {
    volatile unsigned char RSVD0[4100];
    volatile unsigned int EWCTL;
    volatile unsigned int EWINTTCNT;
} ewrap_regs;


/* EMAC MDIO Register Overlay */
typedef volatile struct  {
    volatile unsigned int VERSION;
    volatile unsigned int CONTROL;
    volatile unsigned int ALIVE;
    volatile unsigned int LINK;
    volatile unsigned int LINKINTRAW;
    volatile unsigned int LINKINTMASKED;
    volatile unsigned char RSVD0[8];
    volatile unsigned int USERINTRAW;
    volatile unsigned int USERINTMASKED;
    volatile unsigned int USERINTMASKSET;
    volatile unsigned int USERINTMASKCLEAR;
    volatile unsigned char RSVD1[80];
    volatile unsigned int USERACCESS0;
    volatile unsigned int USERPHYSEL0;
    volatile unsigned int USERACCESS1;
    volatile unsigned int USERPHYSEL1;
} mdio_regs;


#endif  /* _DM644X_EMAC_H_ */

