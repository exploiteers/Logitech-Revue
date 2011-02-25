#
# Makefile for the linux kernel.
#

obj-y					:= core.o clock.o
obj-$(CONFIG_MACH_REALVIEW_EB)		+= realview_eb.o
obj-$(CONFIG_MACH_REALVIEW_PB1176)	+= realview_pb1176.o
obj-$(CONFIG_SMP)			+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o
obj-$(CONFIG_LOCAL_TIMERS)		+= localtimer.o
ifeq ($(CONFIG_PCI),y)
obj-$(CONFIG_MACH_REALVIEW_EB)		+= pci.o
obj-$(CONFIG_MACH_REALVIEW_PB1176)	+= pci.o
endif
