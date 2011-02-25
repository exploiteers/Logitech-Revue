#
# Makefile for the linux kernel.
#

# Common support
obj-y := irq.o clock.o gpio.o wdog.o
obj-$(CONFIG_ARCH_MX2) += dma_mx2.o usb_common.o

# USB support
obj-$(CONFIG_ISP1504_MXC) += isp1504xc.o

obj-$(CONFIG_ISP1301_MXC) += isp1301xc.o

obj-$(CONFIG_MC13783_MXC) += mc13783_xc.o

ifneq ($(CONFIG_USB_EHCI_ARC_H1),)
obj-y += serialxc.o
endif
