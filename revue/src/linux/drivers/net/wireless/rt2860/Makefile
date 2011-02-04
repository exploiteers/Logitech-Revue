obj-$(CONFIG_RT2860) += rt2860sta.o

EXTRA_CFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT -DLINUX
EXTRA_CFLAGS += -DSYSTEM_LOG_SUPPORT -DCONFIG_STA_SUPPORT -DDBG
EXTRA_CFLAGS += -DSTATS_COUNT_SUPPORT
EXTRA_CFLAGS += -DRTMP_MAC_PCI -DRTMP_PCI_SUPPORT -DRT2860 -DRT28xx
EXTRA_CFLAGS += -DWPA_SUPPLICANT_SUPPORT -DNATIVE_WPA_SUPPLICANT_SUPPORT
EXTRA_CFLAGS += -Idrivers/net/wireless/rt2860/include

rt2860sta-objs :=\
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
	os/linux/sta_ioctl.o\
	os/linux/extra_priv_ext.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o\
	common/ba_action.o\
	common/cmm_mac_pci.o\
	common/cmm_data_pci.o\
	common/ee_prom.o\
	common/rtmp_mcu.o\
	os/linux/rt_pci_rbus.o\
	os/linux/rt_rbus_pci_util.o\
	os/linux/pci_main_dev.o
