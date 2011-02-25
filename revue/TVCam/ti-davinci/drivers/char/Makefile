#
# Makefile for the kernel character device drivers.
#

#
# This file contains the font map for the default (hardware) font
#
FONTMAPFILE = cp437.uni

obj-y	 += mem.o random.o tty_io.o n_tty.o tty_ioctl.o

obj-$(CONFIG_LEGACY_PTYS)	+= pty.o
obj-$(CONFIG_UNIX98_PTYS)	+= pty.o
obj-y				+= misc.o
obj-$(CONFIG_VT)		+= vt_ioctl.o vc_screen.o consolemap.o \
				   consolemap_deftbl.o selection.o keyboard.o
obj-$(CONFIG_HW_CONSOLE)	+= vt.o defkeymap.o
obj-$(CONFIG_MAGIC_SYSRQ)	+= sysrq.o
obj-$(CONFIG_ESPSERIAL)		+= esp.o
obj-$(CONFIG_MVME147_SCC)	+= generic_serial.o vme_scc.o
obj-$(CONFIG_MVME162_SCC)	+= generic_serial.o vme_scc.o
obj-$(CONFIG_BVME6000_SCC)	+= generic_serial.o vme_scc.o
obj-$(CONFIG_ROCKETPORT)	+= rocket.o
obj-$(CONFIG_SERIAL167)		+= serial167.o
obj-$(CONFIG_CYCLADES)		+= cyclades.o
obj-$(CONFIG_STALLION)		+= stallion.o
obj-$(CONFIG_ISTALLION)		+= istallion.o
obj-$(CONFIG_DIGIEPCA)		+= epca.o
obj-$(CONFIG_SPECIALIX)		+= specialix.o
obj-$(CONFIG_MOXA_INTELLIO)	+= moxa.o
obj-$(CONFIG_SIBYTE_SB1250_DUART) += sb1250_duart.o
obj-$(CONFIG_A2232)		+= ser_a2232.o generic_serial.o
obj-$(CONFIG_ATARI_DSP56K)	+= dsp56k.o
obj-$(CONFIG_MOXA_SMARTIO)	+= mxser.o
obj-$(CONFIG_COMPUTONE)		+= ip2/
obj-$(CONFIG_RISCOM8)		+= riscom8.o
obj-$(CONFIG_ISI)		+= isicom.o
obj-$(CONFIG_SYNCLINK)		+= synclink.o
obj-$(CONFIG_SYNCLINKMP)	+= synclinkmp.o
obj-$(CONFIG_SYNCLINK_GT)	+= synclink_gt.o
obj-$(CONFIG_N_HDLC)		+= n_hdlc.o
obj-$(CONFIG_AMIGA_BUILTIN_SERIAL) += amiserial.o
obj-$(CONFIG_SX)		+= sx.o generic_serial.o
obj-$(CONFIG_RIO)		+= rio/ generic_serial.o
obj-$(CONFIG_HVC_CONSOLE)	+= hvc_vio.o hvsi.o
obj-$(CONFIG_HVC_ISERIES)	+= hvc_iseries.o
obj-$(CONFIG_HVC_RTAS)		+= hvc_rtas.o
obj-$(CONFIG_HVC_BEAT)		+= hvc_beat.o
obj-$(CONFIG_HVC_DRIVER)	+= hvc_console.o
obj-$(CONFIG_RAW_DRIVER)	+= raw.o
obj-$(CONFIG_SGI_SNSC)		+= snsc.o snsc_event.o
obj-$(CONFIG_MMTIMER)		+= mmtimer.o
obj-$(CONFIG_VIOCONS)		+= viocons.o
obj-$(CONFIG_VIOTAPE)		+= viotape.o
obj-$(CONFIG_HVCS)		+= hvcs.o
obj-$(CONFIG_SGI_MBCS)		+= mbcs.o
obj-$(CONFIG_BRIQ_PANEL)	+= briq_panel.o

obj-$(CONFIG_PRINTER)		+= lp.o
obj-$(CONFIG_TIPAR)		+= tipar.o

obj-$(CONFIG_DTLK)		+= dtlk.o
obj-$(CONFIG_R3964)		+= n_r3964.o
obj-$(CONFIG_APPLICOM)		+= applicom.o
obj-$(CONFIG_SONYPI)		+= sonypi.o
obj-$(CONFIG_RTC)		+= rtc.o
obj-$(CONFIG_HPET)		+= hpet.o
obj-$(CONFIG_GEN_RTC)		+= genrtc.o
obj-$(CONFIG_EFI_RTC)		+= efirtc.o
obj-$(CONFIG_SGI_DS1286)	+= ds1286.o
obj-$(CONFIG_SGI_IP27_RTC)	+= ip27-rtc.o
obj-$(CONFIG_OMAP_RTC)		+= omap-rtc.o
obj-$(CONFIG_DS1302)		+= ds1302.o
obj-$(CONFIG_S3C2410_RTC)	+= s3c2410-rtc.o
ifeq ($(CONFIG_GENERIC_NVRAM),y)
  obj-$(CONFIG_NVRAM)	+= generic_nvram.o
else
  obj-$(CONFIG_NVRAM)	+= nvram.o
endif
obj-$(CONFIG_TOSHIBA)		+= toshiba.o
obj-$(CONFIG_I8K)		+= i8k.o
obj-$(CONFIG_DS1620)		+= ds1620.o
obj-$(CONFIG_HW_RANDOM)		+= hw_random/
obj-$(CONFIG_FTAPE)		+= ftape/
obj-$(CONFIG_COBALT_LCD)	+= lcd.o
obj-$(CONFIG_PPDEV)		+= ppdev.o
obj-$(CONFIG_NWBUTTON)		+= nwbutton.o
obj-$(CONFIG_NWFLASH)		+= nwflash.o
obj-$(CONFIG_SCx200_GPIO)	+= scx200_gpio.o
obj-$(CONFIG_PC8736x_GPIO)	+= pc8736x_gpio.o
obj-$(CONFIG_NSC_GPIO)		+= nsc_gpio.o
obj-$(CONFIG_CS5535_GPIO)	+= cs5535_gpio.o
obj-$(CONFIG_GPIO_VR41XX)	+= vr41xx_giu.o
obj-$(CONFIG_TANBAC_TB0219)	+= tb0219.o
obj-$(CONFIG_TELCLOCK)		+= tlclk.o

obj-$(CONFIG_BLOCKER)		+= blocker.o
obj-$(CONFIG_LPPTEST)		+= lpptest.o

obj-$(CONFIG_WATCHDOG)		+= watchdog/
obj-$(CONFIG_MWAVE)		+= mwave/
obj-$(CONFIG_AGP)		+= agp/
obj-$(CONFIG_DRM)		+= drm/
obj-$(CONFIG_PCMCIA)		+= pcmcia/
obj-$(CONFIG_IPMI_HANDLER)	+= ipmi/

obj-$(CONFIG_HANGCHECK_TIMER)	+= hangcheck-timer.o
obj-$(CONFIG_TCG_TPM)		+= tpm/

davinci_vdce_driver-objs 	:= davinci_vdce.o davinci_vdce_hw.o
obj-$(CONFIG_VDCE)		+= davinci_vdce_driver.o

obj-$(CONFIG_DAVINCI_PWM)	+= davinci_pwm.o

davinci_previewer_driver-objs	:= davinci_previewer_hw.o davinci_previewer.o
obj-$(CONFIG_PREVIEWER)		+= davinci_previewer_driver.o

davinci_rsz_driver-objs		:= davinci_resizer_hw.o davinci_resizer.o
obj-$(CONFIG_RESIZER)		+= davinci_rsz_driver.o

obj-$(CONFIG_ARCH_DAVINCI644x)	+= dm6446_imp.o

dm355_imp-objs			:= dm355_ipipe.o dm355_def_para.o \
					dm355_ipipe_hw.o
obj-$(CONFIG_DM355_IPIPE)	+= dm355_imp.o

dm365_imp-objs			:= dm365_ipipe.o dm365_def_para.o \
					dm365_ipipe_hw.o dm3xx_ipipe.o
obj-$(CONFIG_DM365_IPIPE)	+= dm365_imp.o

dm365_ldc_driver-objs		:= dm365_ldc.o
obj-$(CONFIG_DM365_LDC)		+= dm365_ldc_driver.o

dm365_facedetect_driver-objs	:= dm365_facedetect.o
obj-$(CONFIG_DM365_FACEDETECT)	+= dm365_facedetect_hw.o dm365_facedetect.o

imp_prev_driver-objs		:= imp_previewer.o
imp_rsz_driver-objs		:= imp_resizer.o
obj-$(CONFIG_DM355_IPIPE)	+= imp_common.o
obj-$(CONFIG_DM365_IPIPE)	+= imp_common.o
obj-$(CONFIG_IMP_PREVIEWER)	+= imp_prev_driver.o
obj-$(CONFIG_IMP_RESIZER)	+= imp_rsz_driver.o

obj-$(CONFIG_DM646x_CIR)	+= cir.o
obj-$(CONFIG_DM355_CIR)		+= cir_355.o
obj-$(CONFIG_DM6446_CIR)	+= cir_6446.o

obj-$(CONFIG_DAVINCI_PCI)	+= pcimodule.o

obj-$(CONFIG_DAVINCI_DM646X_TSIF)	+= tsif_control.o tsif_data.o

obj-$(CONFIG_DA8XX_LCD)		+= da8xx_lcd/

# Files generated that shall be removed upon make clean
clean-files := consolemap_deftbl.c defkeymap.c qtronixmap.c

quiet_cmd_conmk = CONMK   $@
      cmd_conmk = scripts/conmakehash $< > $@

$(obj)/consolemap_deftbl.c: $(src)/$(FONTMAPFILE)
	$(call cmd,conmk)

$(obj)/defkeymap.o:  $(obj)/defkeymap.c

$(obj)/qtronixmap.o: $(obj)/qtronixmap.c

# Uncomment if you're changing the keymap and have an appropriate
# loadkeys version for the map. By default, we'll use the shipped
# versions.
# GENERATE_KEYMAP := 1

ifdef GENERATE_KEYMAP

$(obj)/defkeymap.c $(obj)/qtronixmap.c: $(obj)/%.c: $(src)/%.map
	loadkeys --mktable $< > $@.tmp
	sed -e 's/^static *//' $@.tmp > $@
	rm $@.tmp

endif
