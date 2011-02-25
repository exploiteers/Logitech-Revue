

#include <asm-arm/arch-davinci/dm365_deep_sleep.h>

static void (*func_ptr1)(void);

static unsigned int base, ddr_base, other_registers_base, psc_base;

static void run_deep_sleep(void)
{

__asm__("stmfd sp!, {r1-r5}\n\t"
	"MOV R5, #0xF0000000\n\t" /* Not sure if there is a bug in assembler */
	"ADD R5,#0xEA00000 \n\t"  /* MOV R5, #0xFEA00500 gave an assembler error*/	
	"ADD R5,#0x500 \n\t"

/* Power down usb phy */
	"LDR R1, [R5, #0x68]\n\t"
	"LDR R2, [R1]\n\t"
	"ORR R2, R2, #0x00000001\n\t"
	"STR R2, [R1]\n\t"

/* Power down video dac */
	"LDR R1, [R5, #0x6C]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFE7\n\t"
	"STR R2, [R1]\n\t"

/* The following STR instruction was not working for some reason after system wakeup. Letting it here works*/
	"LDR R1, [R5, #0x38]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFE0\n\t"
	"ORR R2, R2, #0x00000001\n\t"	
	"STR R2, [R1]\n\t"

/* Set DDR to self refresh */	
	"LDR R1, [R5, #0xC]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFF7FFFFF\n\t"
	"STR R2, [R1]\n\t"
	"ORR R2, R2, #0x80000000\n\t"	
	"STR R2, [R1]\n\t"
	"ORR R2, R2, #0xC0000000\n\t"	
	"STR R2, [R1]\n\t"

/* Stop PLL */
	"LDR R1, [R5, #0x5C]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFFE\n\t"
	"STR R2, [R1]\n\t"
	"ORR R2, R2, #0x00000002\n\t"	
	"STR R2, [R1]\n\t"

	"LDR R1, [R5, #0x60]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFFE\n\t"
	"STR R2, [R1]\n\t"
	"ORR R2, R2, #0x00000002\n\t"	
	"STR R2, [R1]\n\t"

/* Deep Sleep */
	"LDR R1, [R5, #0x8]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xBFFFFFFF\n\t"
	"STR R2, [R1]\n\t"
	"ORR R2, R2, #0x80000000\n\t"	
	"STR R2, [R1]\n\t"

/* wait for sleep complete bit */
	"sleep_complete:\n\t"
	"LDR R1, [R5, #0x8]\n\t"
	"LDR R2, [R1]\n\t"
	"TST R2, #0x40000000\n\t"
	"BEQ sleep_complete\n\t"

/* delay */
	"MOV R4, #0x100\n\t"
	"delay_after_sleep:\n\t"
	"SUBS R4, R4, #1\n\t"
	"BNE delay_after_sleep\n\t"

	"LDR R1, [R5, #0x8]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0x7FFFFFFF\n\t"
	"STR R2, [R1]\n\t"
	"AND R2, R2, #0xBFFFFFFF\n\t"
	"STR R2, [R1]\n\t"

/* Reset the LPMODEN bit to Zero for exiting from Self refresh */
	"LDR R1, [R5, #0xC]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0x3FFFFFFF\n\t"
	"STR R2, [R1]\n\t"

/* Enable PLL1*/
	"LDR R1, [R5, #0x60]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFFD\n\t"
	"STR R2, [R1]\n\t"
/* delay */
	"MOV R4, #0xFF00\n\t"
	"delay_ppl1:\n\t"
	"SUBS R4, R4, #1\n\t"
	"BNE delay_ppl1\n\t"
		
	"ORR R2, R2, #0x00000001\n\t"	
	"STR R2, [R1]\n\t"

/* Enable PLL2*/
	"LDR R1, [R5, #0x5C]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFFD\n\t"
	"STR R2, [R1]\n\t"

/* delay */
	"MOV R4, #0xFF00\n\t"
	"delay_ppl2:\n\t"
	"SUBS R4, R4, #1\n\t"
	"BNE delay_ppl2\n\t"

	"ORR R2, R2, #0x00000001\n\t"	
	"STR R2, [R1]\n\t"

/* Enable DDR*/
	"LDR R1, [R5, #0x38]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFE0\n\t"
	"ORR R2, R2, #0x00000001\n\t"	
	"STR R2, [R1]\n\t"

	"LDR R1, [R5, #0x30]\n\t"
	"MOV R2, #1\n\t"
	"STR R2, [R1]\n\t"

/* Check DDR*/
//	"DDR_power:\n\t"
//	"LDR R1, [R5, #0x2C]\n\t"
//	"LDR R2, [R1]\n\t"
//	"TST R2, #1\n\t"
//	"BEQ DDR_power\n\t"

//	"DDR_power1:\n\t"
//	"LDR R1, [R5, #0x34]\n\t"
//	"LDR R2, [R1]\n\t"
//	"TST R2, #0x0000001F\n\t"
//	"BEQ DDR_power1\n\t"

/* Enable the DDR clock */
	"LDR R1, [R5, #0x38]\n\t"
	"LDR R2, [R1]\n\t"
	"ORR R2, R2, #0x3\n\t"	
	"STR R2, [R1]\n\t"

	"LDR R1, [R5, #0x30]\n\t"
	"MOV R2, #1\n\t"
	"STR R2, [R1]\n\t"

/* Check DDR Clock*/
	"DDR_clock:\n\t"
	"LDR R1, [R5, #0x2C]\n\t"
	"LDR R2, [R1]\n\t"
	"TST R2, #1\n\t"
	"BEQ DDR_clock\n\t"

	"DDR_clock1:\n\t"
	"LDR R1, [R5, #0x34]\n\t"
	"LDR R2, [R1]\n\t"
	"TST R2, #0x0000001F\n\t"
	"BEQ DDR_clock1\n\t"

/* Power up video dac */
	"LDR R1, [R5, #0x6C]\n\t"
	"LDR R2, [R1]\n\t"
	"ORR R2, R2, #0x00000018\n\t"
	"STR R2, [R1]\n\t"

/* Power up usb phy */
/*	"LDR R1, [R5, #0x68]\n\t"
	"LDR R2, [R1]\n\t"
	"AND R2, R2, #0xFFFFFFFE\n\t"
	"STR R2, [R1]\n\t"
*/
	"ldmfd sp!, {r1-r5}"
	
	);

	return;
}

static int load_IRAM(void)
{
	reg_struct reg_def;
	reg_struct *reg_ptr;
	unsigned int *io_base, *sdram_base, *psc_remapped_base, *other_remapped_base;
	io_base = ioremap_nocache(BASE_IOREMAP, 0xFFF);
	sdram_base = ioremap_nocache(DDR_BASE_IOREMAP, 0xFF);
	psc_remapped_base = ioremap_nocache(PSC_REGISTERS_IOREMAP, 0xFFF);
	other_remapped_base = ioremap_nocache(OTHER_REGISTERS_IOREMAP, 0x26000);
	base = io_base;
	ddr_base = sdram_base;
	psc_base = psc_remapped_base;
	other_registers_base = other_remapped_base;
		
	reg_def.reg0 	= (unsigned int)(other_registers_base + (unsigned int)(IRQ_ENT_REG0 - OTHER_REGISTERS_IOREMAP));
	reg_def.reg1 	= (unsigned int)(other_registers_base + (unsigned int)(IRQ_ENT_REG1 - OTHER_REGISTERS_IOREMAP));

	reg_def.reg2 	= (unsigned int)(base + (unsigned int)(DEEPSLEEP  - BASE_IOREMAP));

	reg_def.reg3 	= (unsigned int)(ddr_base + (unsigned int)(DDR_SDREF	- DDR_BASE_IOREMAP));
	reg_def.reg4 	= (unsigned int)(ddr_base + (unsigned int)(DDR_SDSTAT	- DDR_BASE_IOREMAP));
	reg_def.reg5 	= (unsigned int)(ddr_base + (unsigned int)(DDR_PHYCTL 	- DDR_BASE_IOREMAP));

	reg_def.reg6 	= (unsigned int)(other_registers_base + (unsigned int)(INPSTAT 	 - OTHER_REGISTERS_IOREMAP));
	reg_def.reg7 	= (unsigned int)(other_registers_base + (unsigned int)(GPIOSETREG - OTHER_REGISTERS_IOREMAP));
	reg_def.reg8 	= (unsigned int)(other_registers_base + (unsigned int)(GPIOCLRREG - OTHER_REGISTERS_IOREMAP));
	reg_def.reg9 	= (unsigned int)(other_registers_base + (unsigned int)(GPIO_DIR01 - OTHER_REGISTERS_IOREMAP));
	reg_def.reg10 	= (unsigned int)(base + (unsigned int)(PINMUX3_3 - BASE_IOREMAP));

	reg_def.reg11 	= (unsigned int)(psc_base + (unsigned int)(PTSTAT_1   - PSC_REGISTERS_IOREMAP));
	reg_def.reg12 	= (unsigned int)(psc_base + (unsigned int)(PTCMD      - PSC_REGISTERS_IOREMAP));
	reg_def.reg13 	= (unsigned int)(psc_base + (unsigned int)(MDSTAT_DDR - PSC_REGISTERS_IOREMAP));
	reg_def.reg14 	= (unsigned int)(psc_base + (unsigned int)(MDCTL_DDR  - PSC_REGISTERS_IOREMAP));

	reg_def.reg15 	= (unsigned int)(other_registers_base + (unsigned int)(FIQ0 - OTHER_REGISTERS_IOREMAP));
	reg_def.reg16 	= (unsigned int)(other_registers_base + (unsigned int)(FIQ1 - OTHER_REGISTERS_IOREMAP));
	reg_def.reg17 	= (unsigned int)(other_registers_base + (unsigned int)(IRQ0 - OTHER_REGISTERS_IOREMAP));
	reg_def.reg18 	= (unsigned int)(other_registers_base + (unsigned int)(IRQ1 - OTHER_REGISTERS_IOREMAP));

	reg_def.reg19 	= (unsigned int)(base + (unsigned int)(PINMUX4_4 - BASE_IOREMAP));

	reg_def.reg20 	= (unsigned int)(base + (unsigned int)(SYSTEM_PERI_CLKCTRL - BASE_IOREMAP));

	reg_def.reg21 	= (unsigned int)(base + (unsigned int)(PLL1_SECCTL - BASE_IOREMAP));
	reg_def.reg22 	= (unsigned int)(base + (unsigned int)(PLL2_SECCTL - BASE_IOREMAP));
	reg_def.reg23 	= (unsigned int)(base + (unsigned int)(PLL1_CTL - BASE_IOREMAP));
	reg_def.reg24 	= (unsigned int)(base + (unsigned int)(PLL2_CTL - BASE_IOREMAP));	
	reg_def.reg25 	= (unsigned int)(base + (unsigned int)(PLLCFG0 - BASE_IOREMAP));
    
	reg_def.reg26 	= (unsigned int)(base + (unsigned int)(USB_PHY_CTRL - BASE_IOREMAP));
	reg_def.reg27 	= (unsigned int)(base + (unsigned int)(VPSS_CLK_CTRL - BASE_IOREMAP));

	reg_ptr = &reg_def;

	/*Copy the virtual addresses of the CPU registers to Internal RAM */
	memset((void *)VIRT_RAM_ADDRESS,0x00,150);
	memcpy((void *)VIRT_RAM_ADDRESS,reg_ptr,sizeof(reg_def));

	//printk("RAM_ADDRESS = 0x%x\n",RAM_ADDRESS);
	memset((void *)RAM_ADDRESS,0x00,2500);
	func_ptr1 = &run_deep_sleep;
	memcpy((void *)RAM_ADDRESS,func_ptr1,2300);

	return 0;
}



static int powermng_drv_open(struct inode *inode, struct file *file)
{
   return 0;
}

static int powermng_drv_release(struct inode *inode, struct file *file)
{
   return 0;
}

/* Power Off modules */
static int shutdown_modules()
{ 
	static int i;

	/* TIMER3, SPI1, MMCSD1, McBSP, USB */
	for(i=5;i<=9;i++) 
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* SPI2, RTO */
	for(i=11;i<=12;i++) 
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* Peripheral 13 is DDR which is not switched off here*/

	/* AEMIF */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*14)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*14)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* MMC SD0*/
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*15)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*15)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* TIMER4 */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*17)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*17)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* I2C */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*18)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*18)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* UART0,1 Switched off inside the sleep function */
	/* UART0 */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*19)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*19)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	/* UART1 */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*20)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*20)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/*  HPI, SPI0, PWM0,1,2 */
	for(i=21;i<=25;i++)  
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* TIMER0,1,2, SYSTEM, ARM */
	for(i=28;i<=31;i++)  
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* EMULATION */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*35)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*35)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* SPI3, SPI4 */
	for(i=38;i<=39;i++) 
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* PRTCIF KEYSCAN, ADC */
	for(i=41;i<=43;i++) 
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* Voice */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*44)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*44)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* VDAC_CLKREC, VDAC_CLK */
	for(i=45;i<=46;i++) 
	{
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
		*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*i)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2;
		while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}
	}

	/* VPSSMASTER */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*47)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*47)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* MJCP */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*50)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*50)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3; 
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* HDVICP*/
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*51)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*51)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF2; 
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* SET PTCMD*/
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 120)  - PSC_REGISTERS_IOREMAP)))=1;

	return 0;
}

 /* Power on the following modules */
static int start_modules()
{
	static int i;

	/* I2C*/
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*18)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*18)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* Voice */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*44)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*44)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* VPSSMASTER */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*47)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F;
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*47)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* MJCP */
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*50)  - PSC_REGISTERS_IOREMAP))) |= 0x0000000F; 
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0xA00 + 4*50)  - PSC_REGISTERS_IOREMAP))) &= 0xFFFFFFF3;
	while((*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 0x128)  - PSC_REGISTERS_IOREMAP))) & 0x00000001) != 0){}

	/* SET PTCMD*/
	*((unsigned int*)(psc_base + (unsigned int)((PSC_ADDR + 120)  - PSC_REGISTERS_IOREMAP)))=1;

	return 0;
}

static int powermng_drv_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{

	static void (*jump)(void);
	static unsigned int irq_register0_backup,irq_register1_backup;
	static unsigned int *FIQ0_remapped,*FIQ1_remapped,*IRQ0_remapped,*IRQ1_remapped, *GPIO_DIR01_remapped;
	static unsigned int *IRQ_ENT0_remapped,*IRQ_ENT1_remapped;

	switch(cmd){
		case ENTER_DEEPSLEEP:
			IRQ_ENT0_remapped = (unsigned int)(other_registers_base + (unsigned int)(IRQ_ENT_REG0 - OTHER_REGISTERS_IOREMAP));
			IRQ_ENT1_remapped = (unsigned int)(other_registers_base + (unsigned int)(IRQ_ENT_REG1 - OTHER_REGISTERS_IOREMAP));

			FIQ0_remapped = (unsigned int)(other_registers_base + (unsigned int)(FIQ0 - OTHER_REGISTERS_IOREMAP));
			FIQ1_remapped = (unsigned int)(other_registers_base + (unsigned int)(FIQ1 - OTHER_REGISTERS_IOREMAP));
			IRQ0_remapped = (unsigned int)(other_registers_base + (unsigned int)(IRQ0 - OTHER_REGISTERS_IOREMAP));
			IRQ1_remapped = (unsigned int)(other_registers_base + (unsigned int)(IRQ1 - OTHER_REGISTERS_IOREMAP));

			GPIO_DIR01_remapped = (unsigned int)(other_registers_base + (unsigned int)(GPIO_DIR01 -  OTHER_REGISTERS_IOREMAP));
	
			*GPIO_DIR01_remapped |= 0x00000001; //set the GPIO

			*FIQ0_remapped = 0xFFFFFFFF;	
			*FIQ1_remapped = 0xFFFFFFFF;
			*IRQ0_remapped = 0xFFFFFFFF;	
			*IRQ1_remapped = 0xFFFFFFFF;	

			irq_register0_backup = *IRQ_ENT0_remapped;
			irq_register1_backup = *IRQ_ENT1_remapped;

			*IRQ_ENT0_remapped = 0;
			*IRQ_ENT1_remapped = 0;

			printk("Branching to IRAM\n");
			memset((void *)RAM_ADDRESS,0x00,2500);
			func_ptr1 = &run_deep_sleep;
			memcpy((void *)RAM_ADDRESS,func_ptr1,2300);

			shutdown_modules();
			jump = (void *)RAM_ADDRESS;
			jump();
			start_modules();

			printk("Restoring Interrupts\n"); 
			*IRQ_ENT0_remapped = irq_register0_backup;
			*IRQ_ENT1_remapped = irq_register1_backup;
			break;

		case STOP_MODULES:
			printk("Stopping Modules\n"); 
			shutdown_modules();
			break;

		case START_MODULES:
			printk("Starting Modules\n"); 
			start_modules();
			break;
	}

	return(0);
}

static struct file_operations powermng_drv_fops = {
        .owner   = THIS_MODULE,
        .ioctl   = powermng_drv_ioctl,
        .open    = powermng_drv_open,
        .release = powermng_drv_release,
};

static int deep_sleep_init(void)
{
	unsigned int err;
	load_IRAM();
	err = register_chrdev(POWERMNG_DRV_MAJOR, "powermng", &powermng_drv_fops);
	
   if (err) 
	{
       printk(KERN_WARNING "Registering deep_sleep driver failed, Error No: %d\n", err);
       return -1;
        }
	printk("Deep sleep driver registered\n");

	return 0;
}


static void deep_sleep_exit(void)
{
	printk("Unloading deep sleep driver\n");
	unregister_chrdev(POWERMNG_DRV_MAJOR,"powermng");
}

module_init(deep_sleep_init);
module_exit(deep_sleep_exit);
MODULE_LICENSE("GPL");

