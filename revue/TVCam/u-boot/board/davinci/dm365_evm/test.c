/*
 * (C) Copyright 2009
 * Lone Peak Labs, Inc.
 *
 * 
 */

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <nand.h>
#include <malloc.h>
//#include <miiphy.h>
#include <image.h>
#include <linux/mtd/mtd.h>
#include <configs/davinci_dm365_evm.h>
//#include <net.h>
#include <asm/string.h>
//#include "mmcsd.h"
//#include <i2c.h>
//#include "mmc.h"
#include <asm/errno.h>


//#ifdef hud	// jgb comment out for testing
/* start MEMtest address, start address, stop address, and pattern.   */
#define MEMTESTSTART	0x82000000 
#define MEMTESTSTOP	0x87000000
#define MEMTESTPAT	0x7e7e7e7e
//Address line test
#define ADDRTESTSTART	0x82000000
#define ADDRTESTPAT 	0xDDDDDDDD
#define ADDRANTITESTPAT	0x22222222
//~ /* what block to start with in flash test */
#define FLASHTESTSTART	0x300000 //0x6f000
// address in nand where the kernel is
#define KERNELNANDADDR	0x400000
// where in memory to copy kernel from nand to test crc
#define KERNELCRCTESTSPACE 0x80700000
// device in nand the kernel crc test will use
#define KERNELCRCTESTDEVICE 0


#define PASS_TEST_FLASH_SPEED 1000000UL //1 second

// GPIO Registers
// for mux register
#define GIO25MUX 	0x60000000
#define GIO29MUX 	0x00000030
#define GIO31MUX	0x00000300
#define GIO32MUX	0x00000C00
#define GIO36MUX	0x000C0000


// for GPIO registers
#define GIO25 0x02000000
#define GIO29 0x20000000
#define GIO31	0x80000000
#define GIO32	0x00000001
#define GIO36	0x00000010
#define GIO79	0x00008000
#define GIO80	0x00010000
#define GIO82	0x00040000
#define GIO93	0x20000000

#define PINMUX4	*(volatile unsigned int *)0x01C40010
#define GIOBASE 0x01C67000
#define DIR01 	*(volatile unsigned int *)(GIOBASE+0x10)
#define OUT_DATA01	*(volatile unsigned int *)(GIOBASE+0x14)
#define SET_DATA01	*(volatile unsigned int *)(GIOBASE+0x18)
#define CLR_DATA01	*(volatile unsigned int *)(GIOBASE+0x1c)
#define IN_DATA01	*(volatile unsigned int *)(GIOBASE+0x20)



#define DIR23		*(volatile unsigned int *)(GIOBASE+0x38)
#define OUT_DATA23	*(volatile unsigned int *)(GIOBASE+0x3c)
#define SET_DATA23	*(volatile unsigned int *)(GIOBASE+0x40)
#define CLR_DATA23	*(volatile unsigned int *)(GIOBASE+0x44)
#define IN_DATA23		*(volatile unsigned int *)(GIOBASE+0x48)

// function maps of the GIO pins
// led bits  Red GIO 36  Green GIO32
#define STATUSLED GIO29
#define STATUSOFF 0x1

#define REDLED GIO36
#define REDOFF 0x3
#define GREENLED GIO32
#define GREENOFF 0x5
#define LEDOFF ~(REDLED | GREENLED)
#define VREDLED	GIO82			// red and green hardware pins swapped
#define VREDOFF 0x6
#define VGREENLED GIO80
#define VGREENOFF 0x7
#define VBLUELED GIO79
#define VBLUEOFF 0x9
#define VLEDOFF ~(VREDLED | VGREENLED | VBLUELED)
#define VWHITELED (VREDLED | VGREENLED | VBLUELED)

// BON bit test GIO 25
#define BONTESTPIN GIO25		
 #define PINMUX3 *(volatile unsigned int *)0x01C4000C

//Green Light Flash Speeds
#define SLOW 0
#define FAST 1



/* Global Vars */
DECLARE_GLOBAL_DATA_PTR;

	
/* declare the functions */
static int memory_test (ulong memstart, ulong memstop, ulong testpattern);
int bontestmode(void);
void bon_test(void);
void pos_test(void);


void putled(int led);
void putvled(int led);
void bonPASS_flashLED(void);	
/* externals */



int postestpassed = 0;

#if 0
static int nanddump(nand_info_t *nand, ulong off, u_char *flashtest, int only_oob)
{
	int i;
	u_char *datbuf, *oobbuf;

	datbuf = malloc(nand->writesize + nand->oobsize);
	oobbuf = malloc(nand->oobsize);
	if (!datbuf || !oobbuf) {
		puts("No memory for page buffer\n");
		return 1;
	}
	off &= ~(nand->writesize - 1);
	loff_t addr = (loff_t) off;
	struct mtd_oob_ops ops;
	memset(&ops, 0, sizeof(ops));
	ops.datbuf = datbuf;
	ops.oobbuf = oobbuf; /* must exist, but oob data will be appended to ops.datbuf */
	ops.len = nand->writesize;
	ops.ooblen = nand->oobsize;
	ops.mode = MTD_OOB_RAW;
	i = nand->read_oob(nand, addr, &ops);
	if (i < 0) {
		printf("Error (%d) reading page %08lx\n", i, off);
		free(datbuf);
		free(oobbuf);
		return 1;
	}
	printf("Page %08lx dump:\n", off);
	i = nand->writesize >> 4;
	flashtest = datbuf;
	printf("flashtest = %x\n",*flashtest);
	free(datbuf);
	free(oobbuf);

	return 0;
}
#endif //0






/*
 * Perform a memory test. The complete test loops once or 
	a failure of one of the sub-tests.
 */
static int memory_test (ulong memstart, ulong memstop, ulong testpattern)
{
	vu_long	*addr, *start, *end;
	ulong	val;
	ulong	readback;
	int     rcode = 0, i;


	ulong	incr;
	ulong	pattern;

	pattern = testpattern;
	start = (ulong *)memstart;
	end = (ulong *)memstop;
	/* debug statement */
	printf("memory_test (0x%lX, 0x%lX, 0x%lX)\n", memstart, memstop, pattern);

	
	
	incr = 1;
	for (i=0;i < 1;i++) {
	/* no ctl c	if (ctrlc()) {
			putc ('\n');
			return 1;
		} */

		printf ("\rPattern %08lX  Writing..."
			"%12s"
			"\b\b\b\b\b\b\b\b\b\b",
			pattern, "");

		for (addr=start,val=pattern; addr<end; addr++) {
			WATCHDOG_RESET();
			*addr = val;
			val  += incr;
		}

		puts ("Reading...");

		for (addr=start,val=pattern; addr<end; addr++) {
			WATCHDOG_RESET();
			readback = *addr;
			if (readback != val) {
				printf ("\nMem error @ 0x%08X: "
					"found %08lX, expected %08lX\n",
					(uint)addr, readback, val);
				rcode = 1;
			}
			val += incr;
		}

		/*
		 * Flip the pattern each time to make lots of zeros and
		 * then, the next time, lots of ones.  We decrement
		 * the "negative" patterns and increment the "positive"
		 * patterns to preserve this feature.
		 */
		if(pattern & 0x80000000) {
			pattern = -pattern;	/* complement & increment */
		}
		else {
			pattern = ~pattern;
		}
		incr = -incr;
	}
	return rcode;
}

//Address line test on memory

static int memory_addr_test (ulong memstart, ulong testPattern, ulong antiTestPattern)
{
	vu_long *baseAddress;
	ulong offset;
	ulong testOffset;
	
	ulong pattern;
	ulong antiPattern;
//	ulong mask = 0xfff; // this may need to be modified according to the size of the memory
	ulong mask = 0x1ffffff; // this may need to be modified according to the size of the memory
	
	pattern = testPattern;
	antiPattern = antiTestPattern;
	baseAddress = (ulong *)memstart;
	
	/* debug statement */
	printf("memory_address_test (0x%lX, 0x%lX, 0x%lX)\n", memstart,pattern, antiPattern);

	//write default values to address locations
	for(offset = 1; (offset & mask) != 0; offset <<= 1){
		WATCHDOG_RESET();
		baseAddress[offset] = pattern;
	}
	
	//test for address bits stuck high
	testOffset = 0;
	baseAddress[testOffset] = antiPattern;
	
	for(offset = 1; (offset & mask) != 0; offset <<= 1){
		WATCHDOG_RESET();
		if(baseAddress[offset] != pattern){
			return (ulong)&baseAddress[offset];
		}
	}
	baseAddress[testOffset] = pattern;
	
	//test for address bits stuck low
	for(testOffset = 1; (testOffset & mask) != 0; testOffset <<= 1){
		WATCHDOG_RESET();
		baseAddress[testOffset] = antiPattern;
		if(baseAddress[0] != pattern){
			return (ulong)&baseAddress[testOffset];
		}
		for(offset = 1; (offset & mask) != 0; offset <<= 1){
			WATCHDOG_RESET();
			if(offset != testOffset && baseAddress[offset] != pattern){
				return (ulong)&baseAddress[testOffset];
			}
		}
		
		baseAddress[testOffset] = pattern;
	}
	
	return 0;
}

/* check GPIO pin -- Return 1 for true and 0 for false */
int bontestmode(void)
{
	
	unsigned int test;
	PINMUX3 &= ~(GIO25MUX);	// set GI025 to gio function
	PINMUX4 &= ~(GIO29MUX);	//set,GIO31 to gio function
	DIR01 |= BONTESTPIN;				// input gio 25 test mode selection
	DIR01 &= ~(STATUSLED);			// set green and red pins as outputs
#ifndef FAST_BOOT
	printf("GPIO STATUs PINMUX3 0x%08x, DIR01 0x%08x, IN_DATA01 0x%08x\n",PINMUX3,DIR01,IN_DATA01);
#endif
// Next line Compare only works if the printf is in.  Tried with intermediate variables and no joy.  Some Cache issue.
	if (!(IN_DATA01 & BONTESTPIN)){				// production
#ifndef FAST_BOOT
		printf("BON GPIO Test pin Low\n");
#endif		
		/* check pins */
		return (0);
	 }
	 else {
		 printf("BON GPIO Test pin High\n");
		 return (1);
		}	
}	

/* put LED on GPIO pin */
void putled(int led)
{
//	printf("putled %d\n",led);
	if (led == STATUSLED) {
		SET_DATA01 = STATUSLED;			// assert low for status led
		printf("StatusLED\n");
		return;
	}
	else if (led == STATUSOFF) {
		CLR_DATA01 = STATUSLED;			// set to clr for status led
		printf("StatusLED OFF\n");
		return;
	}
	SET_DATA01 = STATUSLED;		// init to high turn off assume high asserted
	printf("LEDoff\n");
}	



void bonPASS_flashLED(){
	static int ledOn = 0;
	while(1){
		if(ledOn){
			putled(STATUSOFF);
		}
		else{
			putled(STATUSLED);
		}
		ledOn = !(ledOn);
		udelay(PASS_TEST_FLASH_SPEED);
	}		
}

// test the kernel
static int kernel_crc_test (void)
{
// copy kernel from nand to ram to work on
	
	int r;
	size_t cnt;
	nand_info_t *nand;
	ulong offset, addr;

// !!!!!!!!!  for debug until working
//	return(0);
//!!!!!!!!!!  remove when nand_read working correctly

	image_header_t *hdr;
	hdr = (image_header_t *)KERNELCRCTESTSPACE;		// get the header address

	printf("Verify kernel checksums\n");
	offset = (ulong) KERNELNANDADDR;		// where the kernel resides in NAND
	addr = (ulong) KERNELCRCTESTSPACE;		// where the kernel is loaded into RAM
	nand = &nand_info[KERNELCRCTESTDEVICE];		// get the nand device block



	printf("\nLoading from %s, offset 0x%lx\n", nand->name, offset);

	cnt = nand->writesize;
	

	printf("1.nand %x, offset %x, cnt %x, addr %x\n",(unsigned int)nand,(unsigned int) offset, (unsigned int) cnt, (unsigned int)addr);


	r = nand_read_skip_bad(nand, offset, &cnt, (u_char *) addr);
	if (r) {
		puts("** Read error\n");
		return (1);
	}

	switch (genimg_get_format ((void *)addr)) {
	case IMAGE_FORMAT_LEGACY:
		hdr = (image_header_t *)addr;
		image_print_contents (hdr);
		cnt = image_get_image_size (hdr) + nand->writesize;
		break;

	default:
		puts ("** Unknown image type\n");
		return 1;
	}


	/* FIXME: skip bad blocks */
//	printf("2.nand %x, offset %x, cnt %x, addr %x\n",nand, offset, cnt, addr);
	
//	cnt = 1937496;
//	cnt = 1937920;
// debug	printf("2.nand %x, offset %x, cnt %x, addr %x\n",(unsigned int)nand, (unsigned int)offset, (unsigned int)cnt, (unsigned int)addr);
	r = nand_read_skip_bad(nand, offset, &cnt, (u_char *) addr);
	if (r) {
		printf("** Read error r = %d\n",r);
		return 1;
	}


	/* Loading ok, update default load address */

	load_addr = addr;
	hdr = (image_header_t *)addr;
	image_print_contents (hdr);

	if (!image_check_hcrc (hdr)) {
		printf("Kernel header checksum failed\n");
		printf("kernel data checksum failed\n");
		return 1;
	}
	else {
		printf("Kernel header checksum passed\n");
		if (!image_check_dcrc (hdr)) {
			printf ("Kernel data checksum failed\n");
			return 1;
		}
		else {
			printf ("Kernel data checksum passed\n");
			return 0;
		}
	}
		
}



/* bed of nails test routine */
void bon_test(void)
{
	

	ulong address;
	int result = 0;
	if (bontestmode() == 1)		/* test if bon should run */
	{	
		setenv("bootdelay","15"); // delay kernal boot for 15 sec after test
		//putled(LEDOFF);		// turn off the leds
		putled(STATUSLED);		//turn on green led

	/*	do tests */
		if (0 != memory_test (MEMTESTSTART, MEMTESTSTOP, MEMTESTPAT)) {		/* memory test */
		/*	red_LED_on();	*/	/* warn bad memory */
			result = 1;
			printf("Memory Test Failed\n");
		}
		else
			printf("Memory test passed\n");
		
		//Memory Address test
		address = memory_addr_test (ADDRTESTSTART, ADDRTESTPAT, ADDRANTITESTPAT);
		if (0 != address) {		/* memory test */
		/*	red_LED_on();	*/	/* warn bad memory */
			result = 1;
			printf("Memory Address Test Failed at: %x\n",(unsigned int)address);
		}
		else
			printf("Memory Address test passed\n");
// kernel checksum test
		if (0 != kernel_crc_test ()) { 	/* flash test */
		/*	red_LED_on();	*/	/* warn bad memory */
			result = 1;
			printf("Kernel Test Failed\n");
		}
		else
			printf("Kernel test passed\n");


		udelay(1000000UL);//rld debug
		result ? putled(STATUSOFF) : bonPASS_flashLED() ;	// if passed all tests(result =0) flash led, if failed a test turn off status LED.
	}
	else {
		// putled (GREENLED);		// no test is a good test
		return;
	}
}

#define TESTD_PASS 0
#define TESTD_FAIL 1
/* The flash is memory mapped ad 0x02000000 */
#define FLASH_OFFSET_MASK 0x01ffffff
/* Our NAND flash and SD Card block size */
#define TEST_BUF_SIZE     512

typedef struct testd_meminfo {
    unsigned int start_addr;
    unsigned int end_addr;
} TestdMemType;

int  r_index = 0;
int  p_index = 0;
int  fl_index = 0;
int  sd_index = 0;
int  Testd_loop = 1;
static char Testd_status[255];
// RAM   0x80000000 - 0x88000000 128MB 
//  U-Boot occupies RAM at 81080000 - 810b7e80
//  Test U-Boot image is at 810c0000 - 810f7e80
// FLASH 0x02000000 - 0x03ffffff Wasatch is 32MB
// linux image is stored in flash at 0x2050000
TestdMemType RamArray[]={
    {0x81100000, 0x8110ffff},    /* defaults */
    {0x81100000, 0x87ffffff},    /* Min. Max. */
    {0x81110000, 0x86ffffff},
};
#define NUM_RAM_ADDRS 3

TestdMemType FlArray[]={
    {0x0206f000, 0x0206f800},       /* defaults */
    {0x0206f000, 0x03ffffff},       /* Min. Max. */
};
#define NUM_FL_ADDRS  2
//~ TestdMemType SdArray[]={
    //~ {0x00000000, 0x000001ff},       /* defaults */
    //~ {0x00000000, 0x000001ff},       /* Min. Max. */
//~ };
//~ #define NUM_SD_ADDRS  2
unsigned int PatternArray[] = {
    MEMTESTPAT,                     /* default */
    0xa5a5a5a5,
    0x5a5a5a5a,
};
#define NUM_PATTERNS  3


/* display_testd_menu
 * Display the list of test and test options for test diagnostics.
 * Processing of menu selections is performed elsewhere. */
static void display_testd_menu(void) 
{
    printf("\n\n");
    printf("%s", Testd_status);
    printf("     1) RAM TEST\n");
    printf("     2) ROM TEST\n");
    printf("     A) TOGGLE RAM START: %.08X RAM END: %.08X\n", 
            RamArray[r_index].start_addr, RamArray[r_index].end_addr);
    printf("     F) TOGGLE ROM START: %.08X ROM END: %.08X\n", 
            FlArray[fl_index].start_addr, FlArray[fl_index].end_addr);
    printf("     P) TOGGLE DATA PATTERN: %.08X\n", PatternArray[p_index]);
    printf("     L) TOGGLE LOOP OR SINGLE EXECUTION: %s\n", 
            Testd_loop?"LOOP":"SINGLE");

    printf("     Q) QUIT/EXIT TEST DIAGNOSTICS\n");
    printf("\n  Selection: ");
    // Clear status
    Testd_status[0] = '\0';
}

/* testd_get_input
 * return character from terminal input.
 * The call to getc blocks.  */
static char testd_get_input(void)
{
    int c;

    //c = getchar();
    c = getc();
    switch (c) {
        case 0x03:    /* ^C - break		*/
        case 0x1B:    /* ESC */
            break;
        default:
            break;
    }
    return c;
}

/* testd_flash_test
 * start_addr: the memory mapped address of where to start writing
 *             nand flash should be 512-byte aligned.
 * end_addr: the memory mapped address of where to stop writing pattern 
 *           should be 512-byte aligned.
 * pattern: the data pattern to write to flash. Only the last byte of the 
 *          pattern is used.
 * The test writes 512 byte blocks (writesize) and validates the written 
 * flash contents by reading back and comparing the data to the original buffer.
 * The calculated length end_addr - start_addr should be multiple of writesize.*/
static int testd_flash_test (ulong start_addr, ulong end_addr, ulong pattern)
{
    nand_info_t *nand;
    u_char      *nandptr;
    size_t      tot_length;
    size_t      length;
    int         i;
    u_char      *datbuf, *oobbuf;
    struct mtd_oob_ops ops;
    ulong       off;
    u_char      flashtest[TEST_BUF_SIZE];
    int ret;

    nand = &nand_info[nand_curr_device];

    datbuf = malloc(nand->writesize + nand->oobsize);
    oobbuf = malloc(nand->oobsize);
    if (!datbuf || !oobbuf) {
        puts("No memory for page buffer\n");
        return 1;
    }

    tot_length = end_addr - start_addr;
    memset(flashtest, pattern, TEST_BUF_SIZE);
    while(tot_length > 0) {
        length = (TEST_BUF_SIZE < tot_length)? TEST_BUF_SIZE: tot_length;

        ret = nand_write_skip_bad(nand, start_addr&FLASH_OFFSET_MASK, &length, 
                flashtest);            /* write to the nand */
        if (ret != 0)            /* if can't write to the flash */
        {
            free(datbuf);
            free(oobbuf);
            return(ret);        /* on the nand write */
        }
        off = (ulong) start_addr&FLASH_OFFSET_MASK;
        off &= ~(nand->writesize - 1);        // range check offset
        loff_t addr = (loff_t) off;
        memset(&ops, 0, sizeof(ops));        // raw read the data in nand
        ops.datbuf = datbuf;
        ops.oobbuf = oobbuf; /* must exist, but oob data will be appended to ops.datbuf */
        ops.len = nand->writesize;
        ops.ooblen = nand->oobsize;
        ops.mode = MTD_OOB_RAW;
        i = nand->read_oob(nand, addr, &ops);
        if (i != 0) {
            printf("Error (%d) reading page %08lx\n", i, off);
            free(datbuf);
            free(oobbuf);
            return 1;
        }
        i = nand->writesize >> 4;
        nandptr = datbuf;
        if (memcmp ((const char *)nandptr, flashtest, length))
        {
            free(datbuf);
            free(oobbuf);        
            return(TESTD_FAIL);
        }
        /* Advance start addr */
        start_addr += length;
        tot_length -= length;
    }
    free(datbuf);
    free(oobbuf);        
    return(TESTD_PASS);
}



/* testd_toggle_loop
 * toggle the Testd_loop variable, this determines if the test will continuously
 * run until interrupted by an ESC character or if it will just run once.
 */
static void testd_toggle_loop(void)
{
    Testd_loop = Testd_loop ? 0 : 1;
}

/* testd_print_inv
 * Called when a menu selection is not valid.
 */
static void testd_print_inv(void)
{
    sprintf(Testd_status, "\n Invalid Selection\n");
}

/* test_diags
 * This is called if the test mode pin is true
 * It is also made available as a U-Boot command 'testd'
 * cmdtp: not used
 * flag:  not used
 * argc:  not used could be used to pass in optional arguments
 * argv:  not used could be used to pass in optional arguments
 * Display test diagnostics menu and process selections. If a test is run
 * report return status to the display.
 */
//int main(int argc, char * argv)
int test_diags(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int result;
    int loopCount;
    int failCount;
    int stopLoop;
    int done     = 0;
    char c;

    r_index  = 0;
    p_index  = 0;
    fl_index = 0;
    sd_index = 0;
    Testd_status[0] = '\0';

    do {
        display_testd_menu();
        switch(testd_get_input()) {
            case '1':
                failCount = loopCount = 0;
                stopLoop = !Testd_loop;
                do {
                    result = memory_test(RamArray[r_index].start_addr, 
                        RamArray[r_index].end_addr, 
                        PatternArray[p_index]);
                    loopCount++;
                    if(result == TESTD_FAIL) {
                        failCount++;
                    }
                   /* if necessary process the character is it an ESC */
                   if(!stopLoop && tstc()) {
                       c = getc();
                       if(c == 0x1B) {
                           stopLoop = 1;
                       }
                   }
                } while(!stopLoop);
                if(failCount) {
                    sprintf(Testd_status, 
                            "Status: RAM TEST FAIL failed %d of %d\n", 
                            failCount, loopCount );
                }
                else {
                   sprintf(Testd_status, 
                           "Status: RAM TEST PASS %d iterations\n", 
                           loopCount );
                }
                break;
            case 'a':
            case 'A':
                r_index++;
                r_index %= NUM_RAM_ADDRS;
                break;
            case '2':
                failCount = loopCount = 0;
                stopLoop = !Testd_loop;
                do {
                    result = testd_flash_test(FlArray[fl_index].start_addr, 
                            FlArray[fl_index].end_addr,
                            PatternArray[p_index]);
                    loopCount++;
                    if(result == TESTD_FAIL) {
                        failCount++;
                    }
                   /* if necessary process the character is it an ESC */
                   if(!stopLoop && tstc()) {
                       c = getc();
                       if(c == 0x1B) {
                           stopLoop = 1;
                       }
                   }
                } while(!stopLoop);
                if(failCount) {
                    sprintf(Testd_status, 
                            "Status: ROM TEST FAIL failed %d of %d\n", 
                            failCount, loopCount );
                }
                else {
                   sprintf(Testd_status, 
                           "Status: ROM TEST PASS %d iterations\n", 
                           loopCount );
                }
                break;
            case 'f':
            case 'F':
                fl_index = ++fl_index % NUM_FL_ADDRS;
                break;
            case 'p':
            case 'P':
                p_index++;
                p_index %= NUM_PATTERNS;
                break;
            case 'L':
            case 'l':
                testd_toggle_loop();
                break;
            case 'q':
            case 'Q':
                done = 1; // exit loop
                break;
            default:
                testd_print_inv();
                break;
        }
    }
    while(!done);
    printf("\n");
    return 0;
}




/* do_testd_ram 
 * args
 *  cmd_tbl_t: not used
 *  flag: not used
 *  argc: number of arguments passed on command line
 *  argv: array of arguments passed on command line
 * This function tests the ram according to arguments passed on
 * the command line: start_addr end_addr pattern iterations
 * The arguments are optional as they all have defaults. */
int do_testd_ram(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
   vu_long start_addr, end_addr, pattern;
   int    iterations;
   int    result = TESTD_PASS;
   int    i;
   char   c;

   if(argc < 2) {
       start_addr = RamArray[0].start_addr;
   }
   else {
       start_addr = simple_strtoul(argv[1], NULL, 16);
   }

   if(argc < 3) {
       /* Set default */
       end_addr = RamArray[0].end_addr;
   }
   else {
       end_addr = simple_strtoul(argv[2], NULL, 16);
   }

   if(argc < 4) {
      /* Use the default pattern */
      pattern = PatternArray[0];
   } 
   else {
      pattern = simple_strtoul(argv[3], NULL, 16);
   }
   /* Default to 1 iteration if iterations is not passed */
   if(argc < 5) {
      iterations = 1;
   }
   else {
      iterations = simple_strtoul(argv[4], NULL, 10);
   }

   /* Range check start_addr end_addr */
   if(start_addr < RamArray[1].start_addr || start_addr > RamArray[1].end_addr) {
      printf("start_addr %.08x outside of range [%.08x:%.08x]\n", 
            (unsigned int)start_addr, (unsigned int)RamArray[1].start_addr, (unsigned int)RamArray[1].end_addr);
      return 0;
   }
   if(end_addr < RamArray[1].start_addr || end_addr > RamArray[1].end_addr) {
      printf("end_addr %.08x outside of range [%.08x:%.08x]\n", 
            (unsigned int)end_addr, (unsigned int)RamArray[1].start_addr, (unsigned int)RamArray[1].end_addr);
   }
   if(end_addr < start_addr) {
      printf("end_addr %.08x must be greater than start_addr %.08x\n", 
            (unsigned int)end_addr, (unsigned int)start_addr);
      return TESTD_FAIL;
   }

   /* Run Test iterations */
   printf("ROM test from %.08x to %.08x with pattern %.08x for %d iterations\n", 
         (unsigned int)start_addr, (unsigned int)end_addr, (unsigned int)pattern, (unsigned int)iterations);
   if(iterations > 1) {
      printf("Press ESC to exit\n");
   }
   for(i = 0; i < iterations; i++) {
     result = memory_test(start_addr, end_addr, pattern);
     /* check for escape character */
     c = tstc() ? getc() : '\0';
     if((result == TESTD_FAIL) || (c == 0x1B)) {
         break;
     }
   }
   if(result == TESTD_PASS) {
      printf("completed %d iterations successfully\n", i);
   }
   else {
      printf("completed %d iterations test failed\n", i);
   }
   return result;
}

/* do_testd_rom 
 * args
 *  cmd_tbl_t: not used
 *  flag: not used
 *  argc: number of arguments passed on command line
 *  argv: array of arguments passed on command line
 * This function tests the rom according to arguments passed on
 * the command line: start_addr end_addr pattern iterations
 * The arguments are optional as they all have defaults. */
int do_testd_rom(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
   vu_long start_addr, end_addr, pattern;
   int    iterations;
   int    result = TESTD_PASS;
   int    i;
   char   c;

   if(argc < 2) {
       start_addr = FlArray[0].start_addr;
   }
   else {
       start_addr = simple_strtoul(argv[1], NULL, 16);
   }

   if(argc < 3) {
       end_addr = FlArray[0].end_addr;
   }
   else {
       end_addr = simple_strtoul(argv[2], NULL, 16);
   }

   if(argc < 4) {
      /* Use the default pattern */
      pattern = PatternArray[0];
   } 
   else {
      pattern = simple_strtoul(argv[3], NULL, 16);
   }

   /* Default to 1 iteration if iterations is not passed */
   if(argc < 5) {
      iterations = 1;
   }
   else {
      iterations = simple_strtoul(argv[4], NULL, 10);
   }

   /* Range check start_addr end_addr */
   if(start_addr < FlArray[1].start_addr || start_addr > FlArray[1].end_addr) {
      printf("start_addr %.08x outside of range [%.08x:%.08x]\n", 
            (unsigned int)start_addr, (unsigned int)FlArray[1].start_addr, (unsigned int)FlArray[1].end_addr);
      return TESTD_FAIL;
   }
   if(end_addr < FlArray[1].start_addr || end_addr > FlArray[1].end_addr) {
      printf("end_addr %.08x outside of range [%.08x:%.08x]\n", 
            (unsigned int)end_addr, (unsigned int)FlArray[1].start_addr, (unsigned int)FlArray[1].end_addr);
   }
   if(end_addr < start_addr) {
      printf("end_addr %.08x must be greater than start_addr %.08x\n", 
            (unsigned int)end_addr, (unsigned int)start_addr);
      return TESTD_FAIL;
   }

   /* Run Test iterations */
   printf("ROM test from %.08x to %.08x with pattern %.08x for %d iterations\n", 
         (unsigned int)start_addr, (unsigned int)end_addr, (unsigned int)pattern, (unsigned int)iterations);
   if(iterations > 1) {
      printf("Press ESC to exit\n");
   }
   for(i = 0; i < iterations; i++) {
     result = testd_flash_test(start_addr, end_addr, pattern);
     /* check for escape character */
     c = tstc() ? getc() : '\0';
     if((result == TESTD_FAIL) || (c == 0x1B)) {
         break;
     }
   }
   if(result == TESTD_PASS) {
      printf("completed %d iterations successfully\n", i);
   }
   else {
      printf("completed %d iterations test failed\n", i);
   }
   return result;
}



/* Register the U-Boot commands for Test Diagnostic functions 
 * U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) */
U_BOOT_CMD(testd, 1, 0, test_diags,
      "testd - display test diagnostics menu\n", NULL);

U_BOOT_CMD(testd_ram, 5, 0, do_testd_ram,
      "testd_ram - test diags RAM write read compare\n",
      "    test_ram [ start_addr ] [ end_addr ] [ pattern ] [ iterations ]\n"
      "             address range in hexidecimal 81100000 - 86ffffff");

U_BOOT_CMD(testd_rom, 5, 0, do_testd_rom,
      "testd_rom - test diags ROM write read compare\n",
      "    test_rom [ start_addr ] [ end_addr ] [ pattern ] [ iterations ]\n"
      "             address range in hexidecimal 02000000 - 03ffffff");

