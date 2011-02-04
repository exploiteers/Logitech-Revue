###################################################################
 
#Includes Intel Corporation's changes/modifications dated: 04/2008.
#Changed/modified portions - Copyright(c) 2008, Intel Corporation. 
#All rights reserved.

###################################################################

obj-y				:= i386.o init.o

obj-$(CONFIG_PCI_BIOS)		+= pcbios.o
obj-$(CONFIG_PCI_MMCONFIG)	+= mmconfig.o direct.o mmconfig-shared.o
obj-$(CONFIG_PCI_DIRECT)	+= direct.o

pci-y				:= fixup.o
pci-$(CONFIG_ARCH_GEN3)           += intel_media_proc_gen3.o
pci-$(CONFIG_ACPI)		+= acpi.o
pci-y				+= legacy.o irq.o

pci-$(CONFIG_X86_VISWS)		:= visws.o fixup.o
pci-$(CONFIG_X86_NUMAQ)		:= numa.o irq.o

obj-y				+= $(pci-y) common.o early.o
