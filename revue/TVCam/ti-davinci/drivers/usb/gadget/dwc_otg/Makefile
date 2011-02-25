#
# Makefile for DWC_otg Highspeed USB controller driver
#

ifeq ($(CONFIG_USB_DWC_MODE_DEVICE_ONLY),y)
	EXTRA_CFLAGS        += -DDWC_DEVICE_ONLY
endif

obj-$(CONFIG_USB_DWC_OTG)	+= dwc_otg.o

dwc_otg-objs	:= dwc_otg_driver.o dwc_otg_attr.o
dwc_otg-objs	+= dwc_otg_cil.o dwc_otg_cil_intr.o
dwc_otg-objs	+= dwc_otg_pcd.o dwc_otg_pcd_intr.o
dwc_otg-objs	+= dwc_otg_hcd.o dwc_otg_hcd_intr.o dwc_otg_hcd_queue.o
