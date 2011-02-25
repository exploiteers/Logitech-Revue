#
# Makefile for the linux kernel.
#
#
obj-y := clock.o dma.o time.o

# Common objects
obj-$(CONFIG_GENERIC_GPIO)	+= gpio.o
obj-$(CONFIG_SERIAL_8250)	+= serial.o
obj-$(CONFIG_USB_OTG)		+= usb.o
obj-$(CONFIG_DAVINCI_MUX)	+= mux.o
obj-$(CONFIG_TI_DAVINCI_EMAC)	+= i2c-emac.o
obj-$(CONFIG_I2C_DAVINCI)	+= i2c-client.o
