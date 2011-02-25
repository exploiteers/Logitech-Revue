#include <linux/config.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm-arm/arch-davinci/io.h>
#include <asm-arm/arch-davinci/hardware.h>
#include <asm-arm/arch-davinci/edma.h>


#include <asm/hardware.h>
#include <asm/io.h>

#include <linux/linkage.h>
#include <asm/arch/io.h>

#define TCM_VIRTUAL		0xFEA00000
#define RAM_ADDRESS		(TCM_VIRTUAL + 0x00001000)
#define VIRT_RAM_ADDRESS	(TCM_VIRTUAL + 0x00000500)


#define BASE_IOREMAP		0x01C40000
#define DDR_BASE_IOREMAP	0x20000000
#define PSC_REGISTERS_IOREMAP	0x01C41000
#define OTHER_REGISTERS_IOREMAP	0x01C42000

#define IRQ_ENT_REG0	0x01C48018
#define IRQ_ENT_REG1 	0x01C4801C

#define FIQ0			0x01C48000
#define FIQ1			0x01C48004
#define IRQ0			0x01C48008
#define IRQ1			0x01C4800C

#define DDR_SDREF		0x2000000C
#define DDR_SDSTAT		0x20000004
#define DDR_PHYCTL		0x200000E4

#define INPSTAT			0x01C67020
#define GPIOCLRREG		0x01C6701C
#define GPIOSETREG		0x01C67018

#define PLL2_BASE_ADDR		0x01C40C00
#define PLL2_SECCTL		(PLL2_BASE_ADDR + 0x108) 	/*PLL Pre-Divider control Register*/
#define PLL2_CTL		(PLL2_BASE_ADDR + 0x100)

#define PLL1_BASE_ADDR		0x01C40800
#define PLL1_CTL		(PLL1_BASE_ADDR + 0x100)
#define PLL1_SECCTL		(PLL1_BASE_ADDR + 0x108) /*PLL Pre-Divider control Register*/

#define PLLCFG0			0x01C40084
#define PLLCFG1			0x01C40088


#define SYSCTL_BASE		0x01c40000
#define DEEPSLEEP		 (SYSCTL_BASE + 0x4C)  	// Deep sleep register
#define USB_PHY_CTRL  (SYSCTL_BASE + 0x34)
#define VPSS_CLK_CTRL (SYSCTL_BASE + 0x44)  
#define PINMUX3_3			0x01C4000C
#define PINMUX4_4			0x01C40010
#define GPIO_DIR01 		0x01c67010

#define SYSTEM_PERI_CLKCTRL 	0x01c40048

#define LPSC_DDR_EMIF		 13		/*DDR_EMIF LPSC*/

#define PSC_ADDR 		0x01C41000
#define PTCMD			(PSC_ADDR + 0x120)
#define PTSTAT_1       	 	(PSC_ADDR + 0x128)
#define MDSTAT_DDR		(PSC_ADDR + 0x800 + 4*LPSC_DDR_EMIF)
#define MDCTL_DDR		(PSC_ADDR + 0xA00 + 4*LPSC_DDR_EMIF)

#define POWERMNG_DRV_MAJOR      201

#define ENTER_DEEPSLEEP	 1
#define STOP_MODULES	 2
#define START_MODULES	 3

typedef struct {
	unsigned int reg0;
	unsigned int reg1;
	unsigned int reg2;
	unsigned int reg3;
	unsigned int reg4;
	unsigned int reg5;
	unsigned int reg6;
	unsigned int reg7;
	unsigned int reg8;
	unsigned int reg9;
	unsigned int reg10;
	unsigned int reg11;
	unsigned int reg12;
	unsigned int reg13;
	unsigned int reg14;
	unsigned int reg15;
	unsigned int reg16;
	unsigned int reg17;
	unsigned int reg18;
	unsigned int reg19;
	unsigned int reg20;
	unsigned int reg21;
	unsigned int reg22;
	unsigned int reg23;
	unsigned int reg24;
	unsigned int reg25;
	unsigned int reg26;
	unsigned int reg27;
	} reg_struct;

