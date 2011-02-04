obj-$(CONFIG_RT3070) += rt3070sta.o

EXTRA_CFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT -DLINUX
EXTRA_CFLAGS += -DSYSTEM_LOG_SUPPORT -DLED_CONTROL_SUPPORT
EXTRA_CFLAGS += -DKTHREAD_SUPPORT
EXTRA_CFLAGS += -DCONFIG_STA_SUPPORT -DDBG
EXTRA_CFLAGS += -DWPA_SUPPLICANT_SUPPORT -DNATIVE_WPA_SUPPLICANT_SUPPORT
EXTRA_CFLAGS += -DSTATS_COUNT_SUPPORT
EXTRA_CFLAGS += -DRTMP_MAC_USB -DRT30xx -DRT3070 -DRTMP_USB_SUPPORT -DRTMP_TIMER_TASK_SUPPORT 
EXTRA_CFLAGS += -DRTMP_RF_RW_SUPPORT -DRTMP_EFUSE_SUPPORT
EXTRA_CFLAGS += -Idrivers/net/wireless/rt3070/include

rt3070sta-objs :=\
	common/crypt_md5.o\
	common/crypt_sha2.o\
	common/crypt_hmac.o\
	common/crypt_aes.o\
	common/crypt_arc4.o\
	common/mlme.o\
	common/cmm_wep.o\
	common/action.o\
	common/cmm_data.o\
	common/rtmp_init.o\
	common/cmm_tkip.o\
	common/cmm_aes.o\
	common/cmm_sync.o\
	common/eeprom.o\
	common/cmm_sanity.o\
	common/cmm_info.o\
	common/cmm_cfg.o\
	common/cmm_wpa.o\
	common/dfs.o\
	common/spectrum.o\
	common/rtmp_timer.o\
	common/rt_channel.o\
	common/cmm_profile.o\
	common/cmm_asic.o\
	common/cmm_cmd.o\
	sta/assoc.o\
	sta/auth.o\
	sta/auth_rsp.o\
	sta/sync.o\
	sta/sanity.o\
	sta/rtmp_data.o\
	sta/connect.o\
	sta/wpa.o\
	sta/sta_cfg.o\
	common/rtmp_init_inf.o\
	os/linux/rt_profile.o\
	os/linux/extra_priv_ext.o\
	os/linux/sta_ioctl.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o\
	common/cmm_mac_usb.o\
	common/rtusb_io.o\
	common/rtusb_bulk.o\
	common/rtusb_data.o\
	common/cmm_data_usb.o\
	common/ee_prom.o\
	common/ee_efuse.o\
	common/rtmp_mcu.o\
	chips/rt30xx.o\
	common/rt_rf.o\
	chips/rt3070.o\
	common/rtusb_dev_id.o\
	os/linux/rt_usb.o\
	os/linux/rt_usb_util.o\
	os/linux/usb_main_dev.o
