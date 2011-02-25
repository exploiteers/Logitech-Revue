/* ------------------------------------------------------------------------ *
 * i2c-davinci-gpio.c I2C bus over TI DaVinci GPIOs                         *
 * ------------------------------------------------------------------------ *
   Copyright (C) 2009 Chris Coley <ccoley@advancev.com>
   
   Based on older i2c-parport-light.c driver
   Copyright (C) 2003-2004 Jean Delvare
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ------------------------------------------------------------------------ */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <asm/io.h>
// adc:cdc Added the gpio and mux interfaces
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mux.h>

static u16 gpio_scl;
module_param(gpio_scl, ushort, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(gpio_scl, "Index of GPIO to use for SCL");
/*#define DEFAULT_GPIO_SCL    (88)*/
#define DEFAULT_GPIO_SCL    (87)  // changed for PA3 to COUT2 (from COUT3)

static u16 gpio_sda;
module_param(gpio_sda, ushort, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(gpio_sda, "Index of GPIO to use for SDA");
#define DEFAULT_GPIO_SDA    (92)

/* ----- Unified line operation functions --------------------------------- */

static inline void line_set(int gpio,int state)
{
    if (state) {
        /* Release pin so it goes to a "1" */
        gpio_direction_input(gpio);
    } else { 
        /* Drive pin to a "0" */
        gpio_direction_output(gpio, 0);
    }
}

static inline int line_get(int gpio)
{
    /* Read the GPIO pin */
    return (gpio_get_value(gpio));
}

/* ----- I2C algorithm call-back functions and structures ----------------- */

static void davinci_gpio_setscl(void *data, int state)
{
    line_set(gpio_scl,state);
}

static void davinci_gpio_setsda(void *data, int state)
{
    line_set(gpio_sda,state);
}

static int davinci_gpio_getscl(void *data)
{
    return line_get(gpio_scl);
}

static int davinci_gpio_getsda(void *data)
{
    return line_get(gpio_sda);
}

/* Encapsulate the functions above in the correct structure
   Note that getscl will be set to NULL by the attaching code for adapters
   that cannot read SCL back */
static struct i2c_algo_bit_data davinci_gpio_algo_data = {
    .setsda     = davinci_gpio_setsda,
    .setscl     = davinci_gpio_setscl,
    .getsda     = davinci_gpio_getsda,
    .getscl     = davinci_gpio_getscl,
    .udelay     = 3,// 1,
    .mdelay     = 50,
    .timeout    = HZ,
}; 

/* ----- I2c structure ---------------------------------------------------- */

static struct i2c_adapter davinci_gpio_adapter = {
    .owner      = THIS_MODULE,
    .class      = I2C_CLASS_HWMON,
    .id         = I2C_HW_B_LP,
    .algo_data  = &davinci_gpio_algo_data,
    .name       = "DaVinci GPIO I2C Driver",
};

/* Define some offsets from the various module base addresses */
#define		PERI_CLKCTL		    0x48
#define		OCSEL_OFFSET		0x104
#define		OSCDIV1_OFFSET		0x124
#define		CKEN_OFFSET		    0x148

/* setup GPIO for image sensor reset and power*/
static void init_sensor_gpio(void)
{
    unsigned int i;
	
   /* First free up the pin as ccdc claimed it but is not using it */
    davinci_cfg_reg(DM365_VIN_CAM_WEN, PINMUX_FREE);

    /* Then enable CLKOUT0 clock on PINMUX */
    davinci_cfg_reg(DM365_CLKOUT0, PINMUX_RESV);

    /* Configure PLL1 and OSC */
    i = davinci_readl(DAVINCI_SYSTEM_MODULE_BASE + PERI_CLKCTL);
    i &= ~0x01; // Enable CLKOUT0
    davinci_writel(i, DAVINCI_SYSTEM_MODULE_BASE + PERI_CLKCTL);

    /* Setup OSC divisor */
    //i = davinci_readl(DAVINCI_PLL_CNTRL0_BASE + OCSEL_OFFSET);
    davinci_writel(0, DAVINCI_PLL_CNTRL0_BASE + OCSEL_OFFSET);

    /* Enable OSCDIV1 for 24 MHz */
    //i = davinci_readl(DAVINCI_PLL_CNTRL0_BASE + OSCDIV1_OFFSET );
    davinci_writel(0x8000, DAVINCI_PLL_CNTRL0_BASE + OSCDIV1_OFFSET );

    /* Setup the PINMUX for the new board */
    //printk(KERN_INFO "setup GPIO (89 & 30) for image sensor reset and power\n");
    davinci_cfg_reg(DM365_GPIO89, PINMUX_RESV);  // GPIO 89 is SENSOR_RESET
    davinci_cfg_reg(DM365_GPIO30, PINMUX_RESV);  // GPIO 30 is EN_SENSOR to enable power to the sensor 

    /* Enable the imager power supply */
    gpio_direction_output(30,1);
    msleep(10);

    /* OBS CLK Enable */
    i = davinci_readl(DAVINCI_PLL_CNTRL0_BASE + CKEN_OFFSET );
    i |= 0x02; // Enable OBS clock
    davinci_writel(i, DAVINCI_PLL_CNTRL0_BASE + CKEN_OFFSET );
    msleep(10);

    /* Toggle the reset line to the Sensor on the new board */
    gpio_direction_output(89,0);
    msleep(10);
    gpio_direction_output(89,1);
    msleep(10);
}

/* ----- Module loading, unloading and information ------------------------ */

static int __init i2c_davinci_gpio_init(void)
{
    /* If gpio_scl is the same as gpio_sda it is an invalid config so use the defaults */
    if (gpio_scl == gpio_sda) {
        gpio_scl = DEFAULT_GPIO_SCL;
        gpio_sda = DEFAULT_GPIO_SDA;
        printk(KERN_INFO "i2c-davinci_gpio: using default \n\tgpio_scl = %d\n\tgpio_sda = %d\n", 
               gpio_scl, gpio_sda);
    }

    /* Set the pin mux's */
    /* Currently we only support GPIO85 through GPIO92 */
    if (85 <= gpio_scl && gpio_scl <= 92) {
        davinci_cfg_reg(DM365_GPIO85 + gpio_scl - 85, PINMUX_RESV);
    } else {
        printk(KERN_ERR "i2c-davinci_gpio: SCL on unsupported pin\n");
        return -ENODEV;
    }
    if (85 <= gpio_sda && gpio_sda <= 92) {
        davinci_cfg_reg(DM365_GPIO85 + gpio_sda - 85, PINMUX_RESV);
    } else {
        printk(KERN_ERR "i2c-davinci_gpio: SDA on unsupported pin\n");
        return -ENODEV;
    }

    /* Set hardware to a sane state (SCL and SDA high) */
    davinci_gpio_setsda(NULL, 1);
    davinci_gpio_setscl(NULL, 1);

    if (i2c_bit_add_bus(&davinci_gpio_adapter) < 0) {
        printk(KERN_ERR "i2c-davinci_gpio: Unable to register with I2C\n");
        return -ENODEV;
    }
	
    init_sensor_gpio();
	
    return 0;
}

static void __exit i2c_davinci_gpio_exit(void)
{
    i2c_bit_del_bus(&davinci_gpio_adapter);
}

MODULE_AUTHOR("Chris Coley <ccoley@advancev.com>");
MODULE_DESCRIPTION("I2C bus over DaVinci GPIOs");
MODULE_LICENSE("GPL");

module_init(i2c_davinci_gpio_init);
module_exit(i2c_davinci_gpio_exit);
