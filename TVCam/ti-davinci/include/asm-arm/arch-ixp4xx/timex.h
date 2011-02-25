/*
 * linux/include/asm-arm/arch-ixp4xx/timex.h
 * 
 */

#include <asm/hardware.h>

/*
 * We use IXP425 General purpose timer for our timer needs, it runs at 
 * 66.66... MHz. We do a convulted calculation of CLOCK_TICK_RATE b/c the
 * timer register ignores the bottom 2 bits of the LATCH value.
 */
#define FREQ 66666666
#define CLOCK_TICK_RATE (((FREQ / HZ & ~IXP4XX_OST_RELOAD_MASK) + 1) * HZ)

extern u64 ixp4xx_get_cycles(void);
#define mach_read_cycles() ixp4xx_get_cycles()
