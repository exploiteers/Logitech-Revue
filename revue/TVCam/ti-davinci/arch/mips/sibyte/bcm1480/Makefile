obj-y := setup.o irq.o

ifndef CONFIG_HIGH_RES_TIMERS
obj-y	+= time.o
endif

obj-$(CONFIG_SMP)			+= smp.o
obj-$(CONFIG_KGDB_SIBYTE)		+= kgdb_bigsur.o

EXTRA_AFLAGS := $(CFLAGS)
