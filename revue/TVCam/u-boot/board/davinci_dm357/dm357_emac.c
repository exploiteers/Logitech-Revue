/*
 * dm644x_emac.c
 *
 * TI DaVinci (DM644X) EMAC peripheral driver source for DV-EVM
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
 * ver. 1.0: Sep 2005, Anant Gole - Created EMAC version for uBoot.
 * ver  1.1: Nov 2005, Anant Gole - Extended the RX logic for multiple descriptors
 *
 */
 
#include <common.h>
#include <command.h>
#include <net.h>
#include "dm357_emac.h"

#ifdef CONFIG_DRIVER_TI_EMAC

#if (CONFIG_COMMANDS & CFG_CMD_NET)

unsigned int emac_dbg = 0;
#define debug_emac(fmt,args...)	if (emac_dbg) printf (fmt ,##args)

/* EMAC internal functions - called when eth_xxx functions are invoked by the kernel */
static int emac_hw_init (void);
static int emac_open (void);
static int emac_close (void);
static int emac_send_packet (volatile void *packet, int length);
static int emac_rcv_packet (void);

/* The driver can be entered at any of the following entry points */
extern int eth_init (bd_t * bd);
extern void eth_halt (void);
extern int eth_rx (void);
extern int eth_send (volatile void *packet, int length);

int eth_hw_init (void)
{
    return emac_hw_init();
}

int eth_init (bd_t * bd)
{
    return emac_open ();
}

void eth_halt ()
{
    emac_close ();
}

int eth_send (volatile void *packet, int length)
{
    return emac_send_packet (packet, length);
}

int eth_rx ()
{
    return emac_rcv_packet ();
}


static char emac_mac_addr[] = { 0x00, 0x00, 0x5b, 0xee, 0xde, 0xad };

/*
 * This function must be called before emac_open() if you want to override
 * the default mac address.
 */

void emac_set_mac_addr (const char *addr)
{
    int i;

    for (i = 0; i < sizeof (emac_mac_addr); i++) {
        emac_mac_addr[i] = addr[i];
    }
}

/***************************
 * EMAC Global variables
 ***************************/ 
 
/* EMAC Addresses */
static volatile emac_regs* adap_emac = (emac_regs *) EMAC_BASE_ADDR;
static volatile ewrap_regs* adap_ewrap = (ewrap_regs *) EMAC_WRAPPER_BASE_ADDR;
static volatile mdio_regs* adap_mdio = (mdio_regs *) EMAC_MDIO_BASE_ADDR;

/* EMAC descriptors */
static volatile emac_desc *emac_rx_desc = (emac_desc *) (EMAC_WRAPPER_RAM_ADDR + EMAC_RX_DESC_BASE);
static volatile emac_desc *emac_tx_desc = (emac_desc *) (EMAC_WRAPPER_RAM_ADDR + EMAC_TX_DESC_BASE);
static volatile emac_desc *emac_rx_active_head = 0;
static volatile emac_desc *emac_rx_active_tail = 0;
static int emac_rx_queue_active = 0;

/* EMAC link status */
static int emac_link_status = 0; /* 0 = link down, 1 = link up */

/* Receive packet buffers */
static unsigned char emac_rx_buffers[EMAC_MAX_RX_BUFFERS * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)];

/* This function initializes the emac hardware */
static int emac_hw_init (void)
{
    /* Enabling power and reset from outside the module is required */
    return (0);
}

/* Read a PHY register via MDIO inteface */
static int mdio_read(int phy_addr, int reg_num)
{
    adap_mdio->USERACCESS0 = MDIO_USERACCESS0_GO | MDIO_USERACCESS0_WRITE_READ | 
                             ((reg_num & 0x1F) << 21) | 
                             ((phy_addr & 0x1F) << 16);

    /* Wait for command to complete */ 
    while ((adap_mdio->USERACCESS0 & MDIO_USERACCESS0_GO) != 0);    

    return (adap_mdio->USERACCESS0 & 0xFFFF);
}

/* Write to a PHY register via MDIO inteface */
void mdio_write(int phy_addr, int reg_num, unsigned int data)
{
    /* Wait for User access register to be ready */
    while ((adap_mdio->USERACCESS0 & MDIO_USERACCESS0_GO) != 0);    
    
    adap_mdio->USERACCESS0 = MDIO_USERACCESS0_GO | MDIO_USERACCESS0_WRITE_WRITE | 
                             ((reg_num & 0x1F) << 21) | 
                             ((phy_addr & 0x1F) << 16) |
                             (data & 0xFFFF);
}


/* Get PHY link state - this function accepts a PHY mask for the caller to
 * find out if any of the passed PHY addresses is connected
 */
int mdio_get_link_state(unsigned int phy_mask)
{
    unsigned int act_phy, phy_addr = 0, link_state = 0;
    unsigned int config;

    act_phy =  (adap_mdio->ALIVE & phy_mask);
    
    if (act_phy) 
    {            
        /* find the phy number */
        while(act_phy) 
        {
            while(!(act_phy & 0x1)) 
            {
                phy_addr++; 
                act_phy >>= 1;
            }
            /* Read the status register from PHY */
            link_state = ((mdio_read(phy_addr, MII_STATUS_REG) & 0x4) >> 2);
            if(link_state == 1) 
            { 
                /* The link can break off anytime, hence adding the fix for boosting the PHY signal
                 * strength here so that everytime the link is found, this can be done and ensured
                 * that we dont miss it
                 */
                 config = mdio_read(phy_addr, MII_DIGITAL_CONFIG_REG);
                 config |= 0x800;
                 mdio_write(phy_addr, MII_DIGITAL_CONFIG_REG, config);
                 /* Read back to verify */
                 config = mdio_read(phy_addr, MII_DIGITAL_CONFIG_REG);
                 
                break;
            } 
            else 
            {
                /* If no link, go to next phy. */                    
                act_phy >>= 1;
                phy_addr++;  
            }
        }
    }
    return link_state;    
}

/*
 * The kernel calls this function when someone wants to use the device,
 * typically 'ifconfig ethX up'.
 */
static int emac_open (void)
{
    volatile unsigned int *addr;
    unsigned int clkdiv, cnt;
    volatile emac_desc *rx_desc;
    
    debug_emac("+ emac_open\n");
            
    /* Reset EMAC module and disable interrupts in wrapper */
    adap_emac->SOFTRESET = 1;
    while (adap_emac->SOFTRESET != 0);
    adap_ewrap->EWCTL = 0;
    for (cnt=0; cnt < 5; cnt++) {
        clkdiv = adap_ewrap->EWCTL;
    }
    
    rx_desc = emac_rx_desc;

    adap_emac->TXCONTROL = 0x1;
    adap_emac->RXCONTROL = 0x1;
        
    /* Set MAC Addresses & Init multicast Hash to 0 (disable any multicast receive) */
    /* Using channel 0 only - other channels are disabled */
    adap_emac->MACINDEX = 0;
    adap_emac->MACADDRHI = (emac_mac_addr[3] << 24) | (emac_mac_addr[2] << 16) |
                            (emac_mac_addr[1] << 8)  | (emac_mac_addr[0]);
    adap_emac->MACADDRLO = ((emac_mac_addr[5] << 8) | emac_mac_addr[4]);
    
    adap_emac->MACHASH1 = 0;
    adap_emac->MACHASH2 = 0;
    
    /* Set source MAC address - REQUIRED */
    adap_emac->MACSRCADDRHI = (emac_mac_addr[3] << 24) | (emac_mac_addr[2] << 16) |
                              (emac_mac_addr[1] << 8)  | (emac_mac_addr[0]);
    adap_emac->MACSRCADDRLO = ((emac_mac_addr[4] << 8) | emac_mac_addr[5]);
    
    /* Set DMA 8 TX / 8 RX Head pointers to 0 */
    addr = &adap_emac->TX0HDP;
    for( cnt=0; cnt<16; cnt++ )
        *addr++ = 0;
    addr = &adap_emac->RX0HDP;
    for( cnt=0; cnt<16; cnt++ )
        *addr++ = 0;

    /* Clear Statistics (do this before setting MacControl register) */
    addr = &adap_emac->RXGOODFRAMES;
    for( cnt=0; cnt < EMAC_NUM_STATS; cnt++ )
        *addr++ = 0;

    /* No multicast addressing */
    adap_emac->MACHASH1 = 0 ;
    adap_emac->MACHASH2 = 0 ;
    
    /* Create RX queue and set receive process in place */
    emac_rx_active_head = emac_rx_desc;
    for (cnt=0; cnt < EMAC_MAX_RX_BUFFERS; cnt++)
    {
        rx_desc->next = (unsigned int) (rx_desc + 1);
        rx_desc->buffer = &emac_rx_buffers[cnt * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)];
        rx_desc->buff_off_len = EMAC_MAX_ETHERNET_PKT_SIZE;
        rx_desc->pkt_flag_len = EMAC_CPPI_OWNERSHIP_BIT;
        ++rx_desc;
    }
    
    /* Set the last descriptor's "next" parameter to 0 to end the RX desc list */
    --rx_desc;
    rx_desc->next = 0;
    emac_rx_active_tail = rx_desc;
    emac_rx_queue_active = 1;
    
    /* Enable TX/RX */
    adap_emac->RXMAXLEN = EMAC_MAX_ETHERNET_PKT_SIZE;
    adap_emac->RXBUFFEROFFSET = 0;
    
    /* No fancy configs - Use this for promiscous for debug - EMAC_RXMBPENABLE_RXCAFEN_ENABLE */
    adap_emac->RXMBPENABLE = EMAC_RXMBPENABLE_RXBROADEN ;

    /* Enable ch 0 only */
    adap_emac->RXUNICASTSET = 0x1; 
    
    /* Enable MII interface and Full duplex mode */
    adap_emac->MACCONTROL = (EMAC_MACCONTROL_MIIEN_ENABLE | EMAC_MACCONTROL_FULLDUPLEX_ENABLE); 
    
    /* Init MDIO & get link state */
    clkdiv = (EMAC_MDIO_BUS_FREQ / EMAC_MDIO_CLOCK_FREQ) - 1;
    adap_mdio->CONTROL = ((clkdiv & 0xFF) | MDIO_CONTROL_ENABLE | MDIO_CONTROL_FAULT);  
    emac_link_status = mdio_get_link_state(EMAC_MDIO_PHY_MASK);
    
    /* Start receive process */
    adap_emac->RX0HDP = (unsigned int) emac_rx_desc;
    
    debug_emac("- emac_open\n");

    return (1);
}

/* EMAC Channel Teardown */
void emac_ch_teardown(int ch)
{
    volatile unsigned int dly = 0xFF;
    volatile unsigned int cnt;
    
    debug_emac("+ emac_ch_teardown\n");
    
    if (ch == EMAC_CH_TX)
    {
        /* Init TX channel teardown */
        adap_emac->TXTEARDOWN = 1;
        for( cnt = 0; cnt != 0xFFFFFFFC; cnt = adap_emac->TX0CP){
            /* Wait here for Tx teardown completion interrupt to occur 
             * Note: A task delay can be called here to pend rather than 
             * occupying CPU cycles - anyway it has been found that teardown 
             * takes very few cpu cycles and does not affect functionality */
            --dly;
            udelay(1);
            if (dly == 0) break;
        }
        adap_emac->TX0CP = cnt;
        adap_emac->TX0HDP = 0;
    }
    else
    {
        /* Init RX channel teardown */
        adap_emac->RXTEARDOWN = 1;
        for( cnt = 0; cnt != 0xFFFFFFFC; cnt = adap_emac->RX0CP){
            /* Wait here for Tx teardown completion interrupt to occur 
             * Note: A task delay can be called here to pend rather than 
             * occupying CPU cycles - anyway it has been found that teardown 
             * takes very few cpu cycles and does not affect functionality */
            --dly;
            udelay(1);
            if (dly == 0) break;
        }
        adap_emac->RX0CP = cnt;
        adap_emac->RX0HDP = 0;
    }
    
    debug_emac("- emac_ch_teardown\n");
}

/*
 * This is called by the kernel in response to 'ifconfig ethX down'.  It
 * is responsible for cleaning up everything that the open routine
 * does, and maybe putting the card into a powerdown state.
 */
static int emac_close (void)
{
    debug_emac("+ emac_close\n");
    
    emac_ch_teardown(EMAC_CH_TX); /* TX Channel teardown */
    emac_ch_teardown(EMAC_CH_RX); /* RX Channel teardown */

    /* Reset EMAC module and disable interrupts in wrapper */
    adap_emac->SOFTRESET = 1;
    adap_ewrap->EWCTL = 0;
    
    debug_emac("- emac_close\n");
    return (1);
}

static int tx_send_loop = 0;

/*
 * This function sends a single packet on the network and returns
 * positive number (number of bytes transmitted) or negative for error
 */
static int emac_send_packet (volatile void *packet, int length)
{
    int ret_status = -1;
    tx_send_loop = 0;
    
    /* Return error if no link */
    emac_link_status = mdio_get_link_state(EMAC_MDIO_PHY_MASK);
    if (emac_link_status == 0)
    {
        printf("WARN: emac_send_packet: No link\n");
        return (ret_status);
    }
    
    /* Check packet size and if < EMAC_MIN_ETHERNET_PKT_SIZE, pad it up */
    if (length < EMAC_MIN_ETHERNET_PKT_SIZE)
    {
      length = EMAC_MIN_ETHERNET_PKT_SIZE;
    }

    /* Populate the TX descriptor */
    emac_tx_desc->next         = 0;
    emac_tx_desc->buffer       = (unsigned char *)packet;
    emac_tx_desc->buff_off_len = (length & 0xFFFF);
    emac_tx_desc->pkt_flag_len = ((length & 0xFFFF) | 
                                  EMAC_CPPI_SOP_BIT | 
                                  EMAC_CPPI_OWNERSHIP_BIT | 
                                  EMAC_CPPI_EOP_BIT);
    /* Send the packet */
    adap_emac->TX0HDP = (unsigned int) emac_tx_desc;
    
    /* Wait for packet to complete or link down */
    while (1)
    {
        emac_link_status = mdio_get_link_state(EMAC_MDIO_PHY_MASK);
        if (emac_link_status == 0)
        {
            emac_ch_teardown(EMAC_CH_TX);
            return (ret_status);
        }
        if (adap_emac->TXINTSTATRAW & 0x1)
        {
            ret_status = length;
            break;
        }
        ++tx_send_loop;
    }

    return (ret_status);
    
}

/*
 * This function handles receipt of a packet from the network
 */
static int emac_rcv_packet (void)
{
    volatile emac_desc *rx_curr_desc;
    volatile emac_desc *curr_desc;
    volatile emac_desc *tail_desc;
    unsigned int status, ret= -1;
    
    rx_curr_desc = emac_rx_active_head;
    status = rx_curr_desc->pkt_flag_len;
    if ((rx_curr_desc) && ((status & EMAC_CPPI_OWNERSHIP_BIT) == 0))
    {
        if (status & EMAC_CPPI_RX_ERROR_FRAME) {
            /* Error in packet - discard it and requeue desc */
		    printf("WARN: emac_rcv_pkt: Error in packet\n");
        }
        else {
            NetReceive(rx_curr_desc->buffer, (rx_curr_desc->buff_off_len & 0xFFFF));
		    ret = rx_curr_desc->buff_off_len & 0xFFFF;
        }

        /* Ack received packet descriptor */
        adap_emac->RX0CP = (unsigned int) rx_curr_desc;
        curr_desc = rx_curr_desc;
        emac_rx_active_head = rx_curr_desc->next;
        
        if (status & EMAC_CPPI_EOQ_BIT) {
            if (emac_rx_active_head) {
                adap_emac->RX0HDP = (unsigned int) emac_rx_active_head;
            } else {
                emac_rx_queue_active = 0;
                printf("INFO:emac_rcv_packet: RX Queue not active\n");
            }
        }
        
        /* Recycle RX descriptor */        
        rx_curr_desc->buff_off_len = EMAC_MAX_ETHERNET_PKT_SIZE;
        rx_curr_desc->pkt_flag_len = EMAC_CPPI_OWNERSHIP_BIT;
        rx_curr_desc->next = 0;

        if (emac_rx_active_head == 0) {
            printf("INFO: emac_rcv_pkt: active queue head = 0\n");
            emac_rx_active_head = curr_desc;
            emac_rx_active_tail = curr_desc;
            if (emac_rx_queue_active != 0) {
                adap_emac->RX0HDP = (unsigned int) emac_rx_active_head;
                printf("INFO: emac_rcv_pkt: active queue head = 0, HDP fired\n");
                emac_rx_queue_active = 1;
            }
        } else {

            tail_desc = emac_rx_active_tail;
            emac_rx_active_tail = curr_desc;
            tail_desc->next = curr_desc;
            status = tail_desc->pkt_flag_len;
            if (status & EMAC_CPPI_EOQ_BIT) {
                adap_emac->RX0HDP = (unsigned int) curr_desc;
                status &= ~EMAC_CPPI_EOQ_BIT;
                tail_desc->pkt_flag_len = status;
            }
        }   
	    return ret;
    }
    return (0);
}

#endif /* CONFIG_COMMANDS & CFG_CMD_NET */

#endif /* CONFIG_DRIVER_TI_EMAC */

