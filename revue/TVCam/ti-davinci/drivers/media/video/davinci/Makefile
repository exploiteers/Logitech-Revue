#
# Makefile for the davinci video device drivers.
#

# Capture: DaVinci and DM355
obj-$(CONFIG_DAVINCI_CCDC) += ccdc_davinci.o ccdc_common.o
obj-$(CONFIG_DM355_CCDC) += ccdc_dm355.o ccdc_common.o
obj-$(CONFIG_DM365_CCDC) += ccdc_dm365.o ccdc_common.o
davinci_capture-objs := davinci_vpfe.o
obj-$(CONFIG_VIDEO_DAVINCI_CAPTURE) += davinci_capture.o
obj-$(CONFIG_DAVINCI_OSD) += davinci_osd.o davinci_platform.o
obj-$(CONFIG_VIDEO_MT9T001) += mt9t001.o
obj-$(CONFIG_VIDEO_MT9P031) += mt9p031.o
obj-$(CONFIG_VIDEO_MT9D111) += mt9d111.o
obj-$(CONFIG_VIDEO_TVP5146) += tvp514x.o

# Encoder Manager and Encoders
obj-$(CONFIG_DAVINCI_ENC_MNGR)  += davinci_enc_mngr.o
obj-$(CONFIG_DAVINCI_VPBE_ENCODER)      += vpbe_encoder.o
obj-$(CONFIG_DAVINCI_LOGICPD_ENCODER) += logicpd_encoder.o
obj-$(CONFIG_DAVINCI_THS8200_ENCODER) += ths8200_encoder.o
obj-$(CONFIG_DAVINCI_AVNET_ENCODER) += avnetlcd_encoder.o
obj-$(CONFIG_DAVINCI_SPCA2211_ENCODER) += spca2211_encoder.o

obj-$(CONFIG_VIDEO_DAVINCI_DISPLAY) += davinci_display.o

# System Deep Sleep
#dm365_deep_sleep_driver-objs := dm365_deep_sleep.o
obj-$(CONFIG_DM365_DEEP_SLEEP) += dm365_deep_sleep.o

# DaVinci HD
obj-$(CONFIG_VIDEO_DAVINCIHD_VPIF) += vpif.o davincihd_platform.o
obj-$(CONFIG_CAPTURE_DAVINCIHD) += davincihd_capture.o
obj-$(CONFIG_VIDEO_TVP5147) += tvp514x.o
obj-$(CONFIG_VIDEO_TVP7002) += tvp7002.o

obj-$(CONFIG_DISPLAY_DAVINCIHD) += davincihd_display.o
obj-$(CONFIG_VIDEO_ADV7343) += adv7343.o

# H3A and IPIPE
davinci_aew_driver-objs := davinci_aew_hw.o davinci_aew.o
obj-$(CONFIG_AEW) += davinci_aew_driver.o
davinci_af_driver-objs := davinci_af_hw.o davinci_af.o
obj-$(CONFIG_AF) += davinci_af_driver.o

dm355_af_driver-objs := dm355_af_hw.o dm355_af.o
obj-$(CONFIG_DM355_AF) += dm355_af_driver.o
dm355_aew_driver-objs := dm355_aew_hw.o dm355_aew.o
obj-$(CONFIG_DM355_AEW) += dm355_aew_driver.o

dm365_a3_hw_driver-objs := dm365_a3_hw.o
obj-$(CONFIG_DM365_3A_HW) += dm365_a3_hw_driver.o
dm365_af_driver-objs := dm365_af.o
obj-$(CONFIG_DM365_AF) += dm365_af_driver.o
dm365_aew_driver-objs := dm365_aew.o
obj-$(CONFIG_DM365_AEW) += dm365_aew_driver.o

# LOGI
dm365_logi_driver-objs := dm365_logi.o
obj-$(CONFIG_DM365_LOGI) += dm365_logi_driver.o

