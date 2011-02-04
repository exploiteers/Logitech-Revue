/*
 * include/asm-i386/serial.h
 */

/******************************************************************
 
 Includes Intel Corporation's changes/modifications dated: 04/2008.
 Changed/modified portions - Copyright(c) 2008, Intel Corporation. 
 All rights reserved.

******************************************************************/

/*
 * This assumes you have a 1.8432 MHz clock for your UART.
 *
 * It'd be nice if someone built a serial card with a 24.576 MHz
 * clock, since the 16550A is capable of handling a top speed of 1.5
 * megabits/second; but this requires the faster clock.
 */
/*
 * The following code is for Intel Media SOC Gen3 base support.
*/
#ifdef CONFIG_GEN3_UART
/*
 * The Intel Media SOC Gen3 has a high freqency clock.
*/
#define BASE_BAUD ( 14745600 / 16 )
#else
#define BASE_BAUD ( 1843200 / 16 )
#endif

/* Standard COM flags (except for COM4, because of the 8514 problem) */
#ifdef CONFIG_SERIAL_DETECT_IRQ
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ)
#define STD_COM4_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_AUTO_IRQ)
#else
#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)
#define STD_COM4_FLAGS ASYNC_BOOT_AUTOCONF
#endif

/*
 * The following code is for Intel Media SOC Gen3 base support.
*/
#ifdef CONFIG_GEN3_UART
/*
 * Definition of the serial port for Intel Media SOC Gen3.
*/
#define SERIAL_PORT_DFNS			\
	/* UART CLK   PORT IRQ     FLAGS        */			\
	{ 0, BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS },	/* ttyS0 */	\
	{ 0, BASE_BAUD, 0x2F8, 4, STD_COM_FLAGS },	/* ttyS1 */
#else
#define SERIAL_PORT_DFNS			\
	/* UART CLK   PORT IRQ     FLAGS        */			\
	{ 0, BASE_BAUD, 0x3F8, 4, STD_COM_FLAGS },	/* ttyS0 */	\
	{ 0, BASE_BAUD, 0x2F8, 3, STD_COM_FLAGS },	/* ttyS1 */	\
	{ 0, BASE_BAUD, 0x3E8, 4, STD_COM_FLAGS },	/* ttyS2 */	\
	{ 0, BASE_BAUD, 0x2E8, 3, STD_COM4_FLAGS },	/* ttyS3 */
#endif
