#
# Makefile for the linux kernel.
#
#

# Common objects
obj-y := time.o clock.o io.o id.o devices.o dma.o

obj-$(CONFIG_DAVINCI_MUX)	+= mux_cfg.o
obj-$(CONFIG_GENERIC_GPIO)	+= gpio.o
obj-$(CONFIG_SPI_DAVINCI)	+= spi.o
obj-$(CONFIG_TI_CPPI41) 	+= cppi41.o
obj-$(CONFIG_MACH_DA8XX_EVM)	+= board-evm.o reset.o
