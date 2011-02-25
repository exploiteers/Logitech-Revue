/*
 *
 *  Copyright (C) 2004 Texas Instruments.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Modifications:
 * ver. 1.0: Oct 2005, Swaminathan S
 *
 */

#ifndef _SOC_H
#define _SOC_H

#include <asm/arch/types.h>

#define CSL_IDEF_INLINE static inline
/*****************************************************************************\
* Peripheral Instance counts
\*****************************************************************************/

#define CSL_UART_CNT                 3
#define CSL_I2C_CNT                  1
#define CSL_TMR_CNT                  4
#define CSL_WDT_CNT                  1
#define CSL_PWM_CNT                  3
#define CSL_PLLC_CNT                 2
#define CSL_PWR_SLEEP_CTRL_CNT       1
#define CSL_SYS_DFT_CNT              1
#define CSL_INTC_CNT                 1
#define CSL_IEEE1394_CNT             1
#define CSL_USBOTG_CNT               1
#define CSL_ATA_CNT                  1
#define CSL_SPI_CNT                  1
#define CSL_GPIO_CNT                 1
#define CSL_UHPI_CNT                 1
#define CSL_VPSS_REGS_CNT            1
#define CSL_EMAC_CTRL_CNT            1
#define CSL_EMAC_WRAP_CNT            1
#define CSL_EMAC_RAM_CNT             1
#define CSL_MDIO_CNT                 1
#define CSL_EMIF_CNT                 1
#define CSL_NAND_CNT                 1
#define CSL_MCBSP_CNT                1
#define CSL_MMCSD_CNT                1
#define CSL_MS_CNT                   1
#define CSL_DDR_CNT                  1
#define CSL_VLYNQ_CNT                1
#define CSL_PMX_CNT                  1

/*****************************************************************************\
* Peripheral Instance enumeration
\*****************************************************************************/

/** @brief Peripheral Instance for UART  */
#define CSL_UART_1                          (0) /** Instance 1 of UART   */

/** @brief Peripheral Instance for UART  */
#define CSL_UART_2                          (1) /** Instance 2 of UART   */

/** @brief Peripheral Instance for UART  */
#define CSL_UART_3                          (2) /** Instance 3 of UART   */

/** @brief Peripheral Instance for I2C   */
#define CSL_I2C                             (0) /** Instance 1 of I2C    */

/** @brief Peripheral Instance for Tmr0  */
#define CSL_TMR_1                           (0) /** Instance 1 of Tmr    */

/** @brief Peripheral Instance for Tmr1  */
#define CSL_TMR_2                           (1) /** Instance 2 of Tmr    */

/** @brief Peripheral Instance for Tmr2  */
#define CSL_TMR_3                           (2) /** Instance 3 of Tmr    */

/** @brief Peripheral Instance for Tmr3  */
#define CSL_TMR_4                           (3) /** Instance 4 of Tmr    */

/** @brief Peripheral Instance for WDT   */
#define CSL_WDT                             (0) /** Instance of WDT      */

/** @brief Peripheral Instance for PWM   */
#define CSL_PWM_1                           (0) /** Instance 1 of PWM    */

/** @brief Peripheral Instance for PWM   */
#define CSL_PWM_2                           (1) /** Instance 2 of PWM    */

/** @brief Peripheral Instance for PWM   */
#define CSL_PWM_3                           (2) /** Instance 3 of PWM    */

/** @brief Peripheral Instance for PLLC  */
#define CSL_PLLC_1                          (0) /** Instance 1 of PLLC   */

/** @brief Peripheral Instance for PLLC  */
#define CSL_PLLC_2                          (1) /** Instance 2 of PLLC   */

/** @brief Peripheral Instance for CSL_PWR_SLEEP_CTRL  */
#define CSL_PWR_SLEEP_CTRL                  (0) /** Instance 1 of PWR_SLEEP_CTRL    */

/** @brief Peripheral Instance for SYS_DFT  */
#define CSL_SYS_DFT                         (0) /** Instance 1 of SYS_DFT*/

/** @brief Peripheral Instance for INTC   */
#define CSL_INTC                            (0) /** Instance 1 of INTC   */

/** @brief Peripheral Instance for IEEE 1394  */
#define CSL_IEEE1394                       (0) /** Instance 1 of IEEE 1394     */

/** @brief Peripheral Instance for USBOTG  */
#define CSL_USBOTG                          (0) /** Instance 1 of USBOTG */

/** @brief Peripheral Instance for ATA   */
#define CSL_ATA_PRIMARY                     (0) /** Instance 1 of ATA    */

/** @brief Peripheral Instance for ATA   */
#define CSL_ATA_SECONDARY                   (1) /** Instance 2 of ATA    */

/** @brief Peripheral Instance for SPI   */
#define CSL_SPI                             (0) /** Instance 1 of SPI    */

/** @brief Peripheral Instance for GPIO  */
#define CSL_GPIO                            (0) /** Instance 1 of GPIO   */

/** @brief Peripheral Instance for UHPI  */
#define CSL_UHPI                            (0) /** Instance 1 of UHPI   */

/** @brief Peripheral Instance for VPSS_REGS  */
#define CSL_VPSS_REGS                       (0) /** Instance 1 of VPSS_REGS     */

/** @brief Peripheral Instance for EMAC_CTRL  */
#define CSL_EMAC_CTRL                       (0) /** Instance 1 of EMAC_CTRL     */

/** @brief Peripheral Instance for EMAC_WRAP  */
#define CSL_EMAC_WRAP                       (0) /** Instance 1 of EMAC_WRAP     */

/** @brief Peripheral Instance for EMAC_RAM  */
#define CSL_EMAC_RAM                        (0) /** Instance 1 of EMAC_RAM      */

/** @brief Peripheral Instance for MDIO  */
#define CSL_MDIO                            (0) /** Instance 1 of MDIO   */

/** @brief Peripheral Instance for EMIF  */
#define CSL_EMIF                            (0) /** Instance 1 of EMIF   */

/** @brief Peripheral Instance for NAND  */
#define CSL_NAND                            (0) /** Instance 1 of NAND   */

/** @brief Peripheral Instance for MCBSP */
#define CSL_MCBSP                           (0) /** Instance 1 of MCBSP  */

/** @brief Peripheral Instance for MMCSD */
#define CSL_MMCSD                           (0) /** Instance 1 of MMCSD  */

/** @brief Peripheral Instance for MS    */
#define CSL_MS                              (0) /** Instance 1 of MS     */

/** @brief Peripheral Instance for DDR    */
#define CSL_DDR                             (0) /** Instance 1 of DDR    */

/** @brief Peripheral Instance for VLYNQ */
#define CSL_VLYNQ                           (0) /** Instance 1 of VLYNQ  */

/** @brief Peripheral Instance for PMX */
#define CSL_PMX                             (0) /** Instance 1 of PMX    */

/*****************************************************************************\
* Peripheral Base Address
\*****************************************************************************/

#define CSL_UART_1_REGS                  (0x01C20000)
#define CSL_UART_2_REGS                  (0x01C20400)
#define CSL_UART_3_REGS                  (0x01C20800)
#define CSL_I2C_1_REGS                   (0x01C21000)
#define CSL_TMR_1_REGS                   (0x01C21400)
#define CSL_TMR_2_REGS                   (0x01C21400)
#define CSL_TMR_3_REGS                   (0x01C21800)
#define CSL_TMR_4_REGS                   (0x01C21800)
#define CSL_WDT_1_REGS                   (0x01C21C00)
#define CSL_PWM_1_REGS                   (0x01C22000)
#define CSL_PWM_2_REGS                   (0x01C22400)
#define CSL_PWM_3_REGS                   (0x01C22800)
#define CSL_PLLC_1_REGS                  (0x01C40800)
#define CSL_PLLC_2_REGS                  (0x01C40C00)
#define CSL_PWR_SLEEP_CTRL_1_REGS        (0x01C41000)
#define CSL_SYS_DFT_1_REGS               (0x01C42000)
#define CSL_INTC1_REGS                   (0x01C48000)
#define CSL_IEEE1394_1_REGS              (0x01C60000)
#define CSL_USBOTG_1_REGS                (0x01C48000)
#define CSL_ATA_1_REGS                   (0x01C66000)
#define CSL_SPI_1_REGS                   (0x01C66800)
#define CSL_GPIO_1_REGS                  (0x01C67000)
#define CSL_UHPI_1_REGS                  (0x01C67800)
#define CSL_VPSS_REGS_1_REGS             (0x01C70000)
#define CSL_EMAC_CTRL_1_REGS             (0x01C80000)
#define CSL_EMAC_WRAP_1_REGS             (0x01C81000)
#define CSL_EMAC_RAM_1_REGS              (0x01C82000)
#define CSL_MDIO_1_REGS                  (0x01C84000)
#define CSL_EMIF_1_REGS                  (0x01E00000)
#define CSL_NAND_1_REGS                  (0x01E00000)
#define CSL_MCBSP_1_REGS                 (0x01E02000)
#define CSL_MMCSD_1_REGS                 (0x01E10000)
#define CSL_MS_1_REGS                    (0x01E20000)
#define CSL_DDR_1_REGS                   (0x20000000)
#define CSL_VLYNQ_1_REGS                 (0x01E01000)
#define CSL_PMX_1_REGS                   (0x01DEAD00) // TODO: Get correct base address.
#define DAVINCI_ASYNC_EMIF_DATA_CE0_BASE (0x02000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE1_BASE (0x04000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE2_BASE (0x06000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE3_BASE (0x08000000)
#define DAVINCI_VLYNQ_REMOTE_BASE        (0x0c000000)

/* Added for EDMA  */
/** @brief Base address of Channel controller  memory mapped registers */
#define CSL_EDMACC_1_REGS                 (0x01C00000u)
#define CSL_EDMA_1                         0


/*****************************************************************************\
* Interrupt/Exception Counts
\*****************************************************************************/

#define _CSL_INTC_EVENTID__INTC0CNT     (8)      /* ARM exception count     */
#define _CSL_INTC_EVENTID__INTC1CNT     (64)     /* Level-1 Interrupt count */

/**
 * @brief   Count of the number of interrupt-events
 */
#define CSL_INTC_EVENTID_CNT        \
    (_CSL_INTC_EVENTID__INTC0CNT + _CSL_INTC_EVENTID__INTC1CNT)

/*****************************************************************************\
* Interrupt Event IDs
\*****************************************************************************/

#define   _CSL_INTC_EVENTID__SPURIOUS         (0)
#define   _CSL_INTC_EVENTID__INTC1START       (0)

#define   CSL_INTC_EVENTID_VD0           (_CSL_INTC_EVENTID__INTC1START + 0)  /**< VPSS - CCDC      */
#define   CSL_INTC_EVENTID_VD1           (_CSL_INTC_EVENTID__INTC1START + 1)  /**< VPSS - CCDC      */
#define   CSL_INTC_EVENTID_VD2           (_CSL_INTC_EVENTID__INTC1START + 2)  /**< VPSS - CCDC      */
#define   CSL_INTC_EVENTID_HIST          (_CSL_INTC_EVENTID__INTC1START + 3)  /**< VPSS - Histogram */
#define   CSL_INTC_EVENTID_H3A           (_CSL_INTC_EVENTID__INTC1START + 4)  /**< VPSS - AE/AWB/AF */
#define   CSL_INTC_EVENTID_PRVU          (_CSL_INTC_EVENTID__INTC1START + 5)  /**< VPSS - Previewer */
#define   CSL_INTC_EVENTID_RSZ           (_CSL_INTC_EVENTID__INTC1START + 6)  /**< VPSS - Resizer   */
#define   CSL_INTC_EVENTID_VFOC          (_CSL_INTC_EVENTID__INTC1START + 7)  /**< VPSS - Focus     */
#define   CSL_INTC_EVENTID_VENC          (_CSL_INTC_EVENTID__INTC1START + 8)  /**< VPSS - VPBE      */
#define   CSL_INTC_EVENTID_ASQ           (_CSL_INTC_EVENTID__INTC1START + 9)  /**< IMCOP - Sqr      */
#define   CSL_INTC_EVENTID_IMX           (_CSL_INTC_EVENTID__INTC1START + 10) /**< IMCOP - iMX      */
#define   CSL_INTC_EVENTID_VLCD          (_CSL_INTC_EVENTID__INTC1START + 11) /**< IMCOP - VLCD     */
#define   CSL_INTC_EVENTID_USBC          (_CSL_INTC_EVENTID__INTC1START + 12) /**< USB OTG Collector*/
#define   CSL_INTC_EVENTID_EMAC          (_CSL_INTC_EVENTID__INTC1START + 13) /**< CPGMAC Wrapper   */
#define   CSL_INTC_EVENTID_1394          (_CSL_INTC_EVENTID__INTC1START + 14) /**< IEEE1394         */
#define   CSL_INTC_EVENTID_1394WK        (_CSL_INTC_EVENTID__INTC1START + 15) /**< IEEE1394         */
#define   CSL_INTC_EVENTID_CC0           (_CSL_INTC_EVENTID__INTC1START + 16) /**< 3PCC Region 0    */
#define   CSL_INTC_EVENTID_CCERR         (_CSL_INTC_EVENTID__INTC1START + 17) /**< 3PCC Error       */
#define   CSL_INTC_EVENTID_TCERR0        (_CSL_INTC_EVENTID__INTC1START + 18) /**< 3PTC0 Error      */
#define   CSL_INTC_EVENTID_TCERR1        (_CSL_INTC_EVENTID__INTC1START + 19) /**< 3PTC1 Error      */
#define   CSL_INTC_EVENTID_PSCINT        (_CSL_INTC_EVENTID__INTC1START + 20) /**< PSC - ALLINT     */
#define   CSL_INTC_EVENTID_RSVD21        (_CSL_INTC_EVENTID__INTC1START + 21) /**< Reserved         */
#define   CSL_INTC_EVENTID_ATA           (_CSL_INTC_EVENTID__INTC1START + 22) /**< ATA/IDE          */
#define   CSL_INTC_EVENTID_HPIINT        (_CSL_INTC_EVENTID__INTC1START + 23) /**< UHPI             */
#define   CSL_INTC_EVENTID_MBX           (_CSL_INTC_EVENTID__INTC1START + 24) /**< McBSP            */
#define   CSL_INTC_EVENTID_MBR           (_CSL_INTC_EVENTID__INTC1START + 25) /**< McBSP            */
#define   CSL_INTC_EVENTID_MMCSD         (_CSL_INTC_EVENTID__INTC1START + 26) /**< MMC/SD           */
#define   CSL_INTC_EVENTID_SDIO          (_CSL_INTC_EVENTID__INTC1START + 27) /**< MMC/SD           */
#define   CSL_INTC_EVENTID_MS            (_CSL_INTC_EVENTID__INTC1START + 28) /**< Memory Stick     */
#define   CSL_INTC_EVENTID_DDR           (_CSL_INTC_EVENTID__INTC1START + 29) /**< DDR EMIF         */
#define   CSL_INTC_EVENTID_EMIF          (_CSL_INTC_EVENTID__INTC1START + 30) /**< Async EMIF       */
#define   CSL_INTC_EVENTID_VLQ           (_CSL_INTC_EVENTID__INTC1START + 31) /**< VLYNQ            */
#define   CSL_INTC_EVENTID_TIMER0INT12   (_CSL_INTC_EVENTID__INTC1START + 32) /**< Timer 0 - TINT12 */
#define   CSL_INTC_EVENTID_TIMER0INT34   (_CSL_INTC_EVENTID__INTC1START + 33) /**< Timer 0 - TINT34 */
#define   CSL_INTC_EVENTID_TIMER1INT12   (_CSL_INTC_EVENTID__INTC1START + 34) /**< Timer 1 - TINT12 */
#define   CSL_INTC_EVENTID_TIMER1INT34   (_CSL_INTC_EVENTID__INTC1START + 35) /**< Timer 2 - TINT34 */
#define   CSL_INTC_EVENTID_PWM0          (_CSL_INTC_EVENTID__INTC1START + 36) /**< PWM0             */
#define   CSL_INTC_EVENTID_PWM1          (_CSL_INTC_EVENTID__INTC1START + 37) /**< PWM1             */
#define   CSL_INTC_EVENTID_PWM2          (_CSL_INTC_EVENTID__INTC1START + 38) /**< PWM2             */
#define   CSL_INTC_EVENTID_I2C           (_CSL_INTC_EVENTID__INTC1START + 39) /**< I2C              */
#define   CSL_INTC_EVENTID_UART0         (_CSL_INTC_EVENTID__INTC1START + 40) /**< UART0            */
#define   CSL_INTC_EVENTID_UART1         (_CSL_INTC_EVENTID__INTC1START + 41) /**< UART1            */
#define   CSL_INTC_EVENTID_UART2         (_CSL_INTC_EVENTID__INTC1START + 42) /**< UART2            */
#define   CSL_INTC_EVENTID_SPI0          (_CSL_INTC_EVENTID__INTC1START + 43) /**< SPI              */
#define   CSL_INTC_EVENTID_SPI1          (_CSL_INTC_EVENTID__INTC1START + 44) /**< SPI              */
#define   CSL_INTC_EVENTID_WDT           (_CSL_INTC_EVENTID__INTC1START + 45) /**< Timer 3 - TINT12 */
#define   CSL_INTC_EVENTID_DSP0          (_CSL_INTC_EVENTID__INTC1START + 46) /**< DSP Controller   */
#define   CSL_INTC_EVENTID_DSP1          (_CSL_INTC_EVENTID__INTC1START + 47) /**< DSP Controller   */
#define   CSL_INTC_EVENTID_GPIO0         (_CSL_INTC_EVENTID__INTC1START + 48) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO1         (_CSL_INTC_EVENTID__INTC1START + 49) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO2         (_CSL_INTC_EVENTID__INTC1START + 50) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO3         (_CSL_INTC_EVENTID__INTC1START + 51) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO4         (_CSL_INTC_EVENTID__INTC1START + 52) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO5         (_CSL_INTC_EVENTID__INTC1START + 53) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO6         (_CSL_INTC_EVENTID__INTC1START + 54) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIO7         (_CSL_INTC_EVENTID__INTC1START + 55) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIOBNK0      (_CSL_INTC_EVENTID__INTC1START + 56) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIOBNK1      (_CSL_INTC_EVENTID__INTC1START + 57) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIOBNK2      (_CSL_INTC_EVENTID__INTC1START + 58) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIOBNK3      (_CSL_INTC_EVENTID__INTC1START + 59) /**< GPIO             */
#define   CSL_INTC_EVENTID_GPIOBNK4      (_CSL_INTC_EVENTID__INTC1START + 60) /**< GPIO             */
#define   CSL_INTC_EVENTID_COMMTX        (_CSL_INTC_EVENTID__INTC1START + 61) /**< ARMSS            */
#define   CSL_INTC_EVENTID_COMMRX        (_CSL_INTC_EVENTID__INTC1START + 62) /**< ARMSS            */
#define   CSL_INTC_EVENTID_EMU           (_CSL_INTC_EVENTID__INTC1START + 63) /**< E2ICE            */

#define   _CSL_INTC_EVENTID__INTC1END    (_CSL_INTC_EVENTID__INTC1START + _CSL_INTC_EVENTID__INTC1CNT - 1)


#define    _CSL_INTC_EVENTID__INTC0START (_CSL_INTC_EVENTID__INTC1END + 1)

#define    CSL_INTC_EVENTID_RESET        (_CSL_INTC_EVENTID__INTC0START + 0)  /**< the RESET exception vector   */
#define    CSL_INTC_EVENTID_UNDEF        (_CSL_INTC_EVENTID__INTC0START + 1)  /**< the UNDEF exception vector   */
#define    CSL_INTC_EVENTID_SWI          (_CSL_INTC_EVENTID__INTC0START + 2)  /**< the SWI exception vector     */
#define    CSL_INTC_EVENTID_PREABT       (_CSL_INTC_EVENTID__INTC0START + 3)  /**< the PREABT exception vector  */
#define    CSL_INTC_EVENTID_DATABT       (_CSL_INTC_EVENTID__INTC0START + 4)  /**< the DATABT exception vector  */
#define    CSL_INTC_EVENTID_IRQ          (_CSL_INTC_EVENTID__INTC0START + 6)  /**< the IRQ exception vector     */
#define    CSL_INTC_EVENTID_FIQ          (_CSL_INTC_EVENTID__INTC0START + 7)  /**< the FIQ exception vector     */

#define    _CSL_INTC_EVENTID__INTC0END   (_CSL_INTC_EVENTID__INTC0START + _CSL_INTC_EVENTID__INTC0CNT - 1)

#define    CSL_INTC_EVENTID_INVALID      (-1)                                 /**< Invalid Event-ID */

#endif

