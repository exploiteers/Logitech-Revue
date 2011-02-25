#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
# David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
#
# (C) Copyright 2003
# Texas Instruments, <www.ti.com>
# Swaminathan <swami.iyer@ti.com>
#
# Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
#
# (C) Copyright 2008
# Sekhar Nori, Texas Instruments, Inc. <nsekhar@ti.com>
#
# Texas Instruments DA8xx EVM board (ARM925EJS) cpu
# see http://www.ti.com/ for more information on Texas Instruments
#
# DA8xx EVM has 1 bank of 64 MB SDRAM (2 16Meg x16 chips).
# Physical Address:
# C000'0000 to C400'0000
#
# Linux-Kernel is expected to be at C000'8000, entry C000'8000
# (mem base + reserved)
#
# we load ourself to C108 '0000
#
#

#Provide at least 16MB spacing between us and the Linux Kernel image
TEXT_BASE = 0xC1080000
