#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
# David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
#
# (C) Copyright 2003
# Texas Instruments, <www.ti.com>
#
# Davinci DM6467 EVM board (ARM925EJS) cpu
# see http://www.ti.com/ for more information on Texas Instruments
#
# Davinci DM6467 EVM base board provides 1 bank of 64M x 32 bit words (256 MB)
# DDR2 SDRAM (Has support up to 512 MB)
# Physical Address:
# 0x8000'0000 to 0x9000'0000
#
# Linux-Kernel is expected to be at 0x8000'8000, entry 0x8000'8000
# (mem base + reserved)
#
# we load ourself to 0x8100'0000
#
#
# Provide atleast 16MB spacing between us and the Linux Kernel image

TEXT_BASE = 0x81080000
BOARD_LIBS = drivers/nand/libnand.a
