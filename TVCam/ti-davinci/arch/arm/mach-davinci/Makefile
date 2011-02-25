#
# Makefile for the linux kernel.
#
#

# Common objects
obj-y := time.o irq.o clock.o io.o id.o dma.o devices.o reset.o davinci_spi_platform.o

# Board specific
obj-$(CONFIG_MACH_DAVINCI_EVM)		+= board-evm.o video_davinci_evm.o vpss.o cpld.o
obj-$(CONFIG_MACH_DAVINCI_DM355_EVM)	+= board-dm355-evm.o video_davinci_evm.o vpss.o cpld.o
obj-$(CONFIG_MACH_DAVINCI_DM6467_EVM)	+= board-dm6467-evm.o video_hdevm.o
obj-$(CONFIG_MACH_DAVINCI_DM357_EVM)	+= board-dm357-evm.o
obj-$(CONFIG_MACH_DAVINCI_DM365_EVM)	+= board-dm365-evm.o video_davinci_evm.o vpss.o cpld.o davinci_keypad.o
obj-$(CONFIG_MACH_DAVINCI_DM365_IPNC)	+= board-dm365-ipnc.o vpss.o device_version.o 
obj-$(CONFIG_MACH_DAVINCI_WOLVERINE)	+= board-wolverine.o video_davinci_evm.o vpss.o cpld.o davinci_keypad.o
obj-$(CONFIG_GENERIC_GPIO)		+= gpio.o
obj-$(CONFIG_DAVINCI_MUX)		+= mux_cfg.o

# Vlynq specific
obj-$(CONFIG_DAVINCI_VLYNQ)		+= davinci_vlynq_init.o
