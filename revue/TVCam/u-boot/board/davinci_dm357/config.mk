#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
# David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
#
# (C) Copyright 2003
# Texas Instruments, <www.ti.com>
# Swaminathan <swami.iyer@ti.com>
#
# Davinci EVM board (ARM925EJS) cpu
# see http://www.ti.com/ for more information on Texas Instruments
#
# Davinci EVM has 1 bank of 256 MB DDR RAM 
# Physical Address:
# 8000'0000 to 9000'0000
#
#
# Linux-Kernel is expected to be at 8000'8000, entry 8000'8000
# (mem base + reserved)
#
# we load ourself to 8100 '0000
#
#

#Provide a atleast 16MB spacing between us and the Linux Kernel image
ifndef UBOOT_START
TEXT_BASE = 0x82080000
else
TEXT_BASE = $(UBOOT_START)
endif
ifndef SDRAM_OFFSET
SDRAM_OFFSET  = 0x1000000
else
SDRAM_OFFSET  = $(SDRAM_OFFSET)
endif
PLATFORM_CPPFLAGS += -DSDRAM_OFFSET=$(SDRAM_OFFSET)
BOARDLIBS = drivers/nand/libnand.a

