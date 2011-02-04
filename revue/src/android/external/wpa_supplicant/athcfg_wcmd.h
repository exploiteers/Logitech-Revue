/*
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 */
#ifndef _ADF_WIOCTL_PVT_H
#define _ADF_WIOCTL_PVT_H

/**
 * Defines
 */
#define a_int32_t   signed int 
#define a_uint8_t   unsigned char     
#define a_uint16_t  unsigned short int
#define a_uint64_t  unsigned long long
#define a_uint32_t  unsigned int
#define a_int8_t    signed char
#define a_int16_t   signed short int


#define ATHCFG_WCMD_NAME_SIZE          16 /**< Max Device name size*/
#define ATHCFG_WCMD_NICK_NAME          32 /**< Max Device nick name size*/     
#define ATHCFG_WCMD_MODE_NAME_LEN      6 
#define ATHCFG_WCMD_IE_MAXLEN          256 /** Max Len for IE */

#define ATHCFG_WCMD_MAX_BITRATES       32
#define ATHCFG_WCMD_MAX_ENC_SZ         8
#define ATHCFG_WCMD_MAX_FREQ           32
#define ATHCFG_WCMD_MAX_TXPOWER        8
#define ATHCFG_WCMD_EVENT_CAP          6

/**
 * @brief key set/get info
 */
#define ATHCFG_WCMD_KEYBUF_SIZE        16
#define ATHCFG_WCMD_MICBUF_SIZE        16/**< space for tx+rx keys */ 
#define ATHCFG_WCMD_KEY_DEFAULT        0x80/**< default xmit key */
#define ATHCFG_WCMD_ADDR_LEN           6
#define ATHCFG_ETH_LEN                 6
#define ATHCFG_WCMD_MAC_STR_LEN     17
#define ATHCFG_WCMD_KEYDATA_SZ          \
    (ATHCFG_WCMD_KEYBUF_SIZE + ATHCFG_WCMD_MICBUF_SIZE)

/**
 *  @brief key flags
 */
#define ATHCFG_WCMD_VAPKEY_XMIT        0x01/**< xmit */
#define ATHCFG_WCMD_VAPKEY_RECV        0x02/**< recv */
#define ATHCFG_WCMD_VAPKEY_GROUP       0x04/**< WPA group*/ 
#define ATHCFG_WCMD_VAPKEY_SWCRYPT     0x10/**< Encrypt/decrypt*/ 
#define ATHCFG_WCMD_VAPKEY_SWMIC       0x20/**< Enmic/Demic */
#define ATHCFG_WCMD_VAPKEY_DEFAULT     0x80/**< Default key */

#define ATHCFG_WCMD_MAX_SSID           32
#define ATHCFG_WCMD_CHAN_BYTES         32

#define ATHCFG_WCMD_RTS_DEFAULT        512 
#define ATHCFG_WCMD_RTS_MIN            1    
#define ATHCFG_WCMD_RTS_MAX            2346 

#define ATHCFG_WCMD_FRAG_MIN           256 
#define ATHCFG_WCMD_FRAG_MAX           2346

/**
 *  @brief  Maximum number of address that you may get in the
 *          list of access ponts
 */
#define  ATHCFG_WCMD_MAX_AP            16 

#define ATHCFG_WCMD_RATE_MAXSIZE       30 
#define ATHCFG_WCMD_NUM_TR_ENTS        128
/**
 * @brief Ethtool specific
 */
#define ATHCFG_WCMD_BUSINFO_LEN        32
#define ATHCFG_WCMD_DRIVSIZ            32  
#define ATHCFG_WCMD_VERSIZ             32  
#define ATHCFG_WCMD_FIRMSIZ            32  

/**
 * @brief flags for encoding (along with the token)
 */
#define ATHCFG_WCMD_ENC_INDEX           0x00FF /* Token index */
#define ATHCFG_WCMD_ENC_FLAGS           0xFF00 /* Flags deifned below */
#define ATHCFG_WCMD_ENC_MODE            0xF000 /* Modes defined below */
#define ATHCFG_WCMD_ENC_DISABLED        0x8000 /* Encoding disabled */
#define ATHCFG_WCMD_ENC_ENABLED         0x0000 /* Encoding enabled */
#define ATHCFG_WCMD_ENC_RESTRICTED      0x4000 /* Refuse nonencoded packets */
#define ATHCFG_WCMD_ENC_OPEN            0x2000 /* Accept non encoded packets */
#define ATHCFG_WCMD_ENC_NOKEY           0x0800 /* Key is write only,*/ 
                                               /* so not preset */
#define ATHCFG_WCMD_ENC_TEMP            0x0400 /* Temporary key */

#ifdef OS_TYPE_LINUX
#define ATHCFG_WCMD_CWMTYPE_INFO    SIOCDEVPRIVATE+2
#define ATHCFG_WCMD_CWMTYPE_DBG     SIOCDEVPRIVATE+3
#else
#define ATHCFG_WCMD_CWMTYPE_INFO    139
#define ATHCFG_WCMD_CWMTYPE_DBG     140
#endif

#define ATHCFG_CAPINFO_ESS_IBSS        (3 << 0)
#define ATHCFG_CAPINFO_CF_PBITS        (3 << 2)
#define ATHCFG_CAPINFO_PRIV            (1 << 4)
#define ATHCFG_CAPINFO_SHORT_PBL       (1 << 5)
#define ATHCFG_CAPINFO_PBCC            (1 << 6)
#define ATHCFG_CAPINFO_CHAN_AGTY       (1 << 7)

#define ATHCFG_WCMD_MAX_HT_MCSSET       16 /* 128-bit */

#define ATHCFG_WCMD_DBG_MAXLEN      24

#define ATHCFG_WCMD_TX_POWER_TABLE_SIZE 36
/**
 * *******************************Enums******************
 */
typedef enum athcfg_wcmd_vapmode{           
    ATHCFG_WCMD_VAPMODE_AUTO,   /**< Driver default*/
    ATHCFG_WCMD_VAPMODE_ADHOC,  /**< Single cell*/
    ATHCFG_WCMD_VAPMODE_INFRA,  /**< Multi Cell or Roaming*/
    ATHCFG_WCMD_VAPMODE_MASTER, /**< Access Point*/
    ATHCFG_WCMD_VAPMODE_REPEAT, /**< Wireless Repeater*/
    ATHCFG_WCMD_VAPMODE_SECOND, /**< Secondary master or repeater*/
    ATHCFG_WCMD_VAPMODE_MONITOR /**< Passive Monitor*/
}athcfg_wcmd_vapmode_t;
/**
 *  @brief key type
 */
typedef enum athcfg_wcmd_ciphermode{
    ATHCFG_WCMD_CIPHERMODE_WEP,
    ATHCFG_WCMD_CIPHERMODE_TKIP,
    ATHCFG_WCMD_CIPHERMODE_AES_OCB,
    ATHCFG_WCMD_CIPHERMODE_AES_CCM ,
    ATHCFG_WCMD_CIPHERMODE_RESERVE,
    ATHCFG_WCMD_CIPHERMODE_CKIP,
    ATHCFG_WCMD_CIPHERMODE_NONE
}athcfg_wcmd_ciphermode_t;
/**
 * @brief Get/Set wireless commands
 */
typedef enum athcfg_wcmd_type{
    /* net80211 */
    ATHCFG_WCMD_GET_RTS_THRES,     
    ATHCFG_WCMD_SET_RTS_THRES,     
    ATHCFG_WCMD_GET_FRAGMENT,  
    ATHCFG_WCMD_SET_FRAGMENT,  
    ATHCFG_WCMD_GET_VAPMODE,   
    ATHCFG_WCMD_SET_VAPMODE,
    ATHCFG_WCMD_SET_VAPDELETE,
    ATHCFG_WCMD_GET_BSSID, 
    ATHCFG_WCMD_SET_BSSID,
    ATHCFG_WCMD_GET_NICKNAME,      
    ATHCFG_WCMD_SET_NICKNAME,      
    ATHCFG_WCMD_GET_FREQUENCY,     
    ATHCFG_WCMD_SET_FREQUENCY,    
    ATHCFG_WCMD_GET_ESSID,
    ATHCFG_WCMD_SET_ESSID, 
    ATHCFG_WCMD_GET_TX_POWER,  
    ATHCFG_WCMD_SET_TX_POWER,
    ATHCFG_WCMD_GET_PARAM,
    ATHCFG_WCMD_SET_PARAM,
    ATHCFG_WCMD_GET_OPT_IE,
    ATHCFG_WCMD_SET_OPT_IE,
    ATHCFG_WCMD_GET_APP_IE_BUF,
    ATHCFG_WCMD_SET_APP_IE_BUF,
    ATHCFG_WCMD_GET_ENC,
    ATHCFG_WCMD_SET_ENC,
    ATHCFG_WCMD_GET_KEY,
    ATHCFG_WCMD_SET_KEY,
    ATHCFG_WCMD_GET_SCAN,      
    ATHCFG_WCMD_SET_SCAN,      
    ATHCFG_WCMD_GET_MODE,  
    ATHCFG_WCMD_SET_MODE,  
    ATHCFG_WCMD_GET_CHAN_LIST, 
    ATHCFG_WCMD_SET_CHAN_LIST, 
    ATHCFG_WCMD_GET_WMM_PARAM, 
    ATHCFG_WCMD_SET_WMM_PARAM, 
    ATHCFG_WCMD_GET_VAPNAME,
    ATHCFG_WCMD_GET_IC_CAPS,
    ATHCFG_WCMD_GET_RETRIES,
    ATHCFG_WCMD_GET_WAP_LIST,
    ATHCFG_WCMD_GET_ADDBA_STATUS,
    ATHCFG_WCMD_GET_CHAN_INFO,
    ATHCFG_WCMD_GET_WPA_IE,
    ATHCFG_WCMD_GET_WSC_IE,
    ATHCFG_WCMD_SET_TXPOWER_LIMIT,
    ATHCFG_WCMD_SET_TURBO,
    ATHCFG_WCMD_SET_FILTER,
    ATHCFG_WCMD_SET_ADDBA_RESPONSE,
    ATHCFG_WCMD_SET_MLME,
    ATHCFG_WCMD_SET_SEND_ADDBA,
    ATHCFG_WCMD_SET_SEND_DELBA,
    ATHCFG_WCMD_SET_DELKEY_ALL,
    ATHCFG_WCMD_SET_DELKEY,
    ATHCFG_WCMD_SET_DELMAC,
    ATHCFG_WCMD_SET_ADD_MAC,
    ATHCFG_WCMD_GET_RANGE,
    ATHCFG_WCMD_GET_POWER,
    ATHCFG_WCMD_SET_POWER,
    ATHCFG_WCMD_GET_DEVSTATS, /* stats_dev */
    ATHCFG_WCMD_SET_MTU,
    ATHCFG_WCMD_SET_SYSCTL,
    ATHCFG_WCMD_GET_STA_STATS,/* stats_sta */
    ATHCFG_WCMD_GET_VAP_STATS, /* stats_vap */
    ATHCFG_WCMD_GET_STATION_LIST, /* station */
    ATHCFG_WCMD_SET_SCANDELAY,
    ATHCFG_WCMD_GET_SCANDELAY,
    ATHCFG_WCMD_SET_AUTOASSOC,
    ATHCFG_WCMD_GET_AUTOASSOC,
    /* Device specific */
    ATHCFG_WCMD_SET_DEV_VAP_CREATE,
    ATHCFG_WCMD_SET_DEV_TX_TIMEOUT,        
    ATHCFG_WCMD_SET_DEV_MODE_INIT,        
    ATHCFG_WCMD_GET_DEV_STATUS,            /* stats_phy */ 
    ATHCFG_WCMD_GET_DEV_STATUS_CLR,        
    ATHCFG_WCMD_GET_DEV_DIALOG,
    ATHCFG_WCMD_GET_DEV_PHYERR,
    ATHCFG_WCMD_GET_DEV_CWM,
    ATHCFG_WCMD_GET_DEV_ETHTOOL,       
    ATHCFG_WCMD_SET_DEV_MAC,
    ATHCFG_WCMD_SET_DEV_CAP,/*ATH_CAP*/
    ATHCFG_WCMD_GET_DEV_PRODUCT_INFO,
    /* Device write specific */
    ATHCFG_WCMD_SET_DEV_EIFS_MASK,
    ATHCFG_WCMD_SET_DEV_EIFS_DUR,
    ATHCFG_WCMD_SET_DEV_SLOTTIME,
    ATHCFG_WCMD_SET_DEV_ACKTIMEOUT,
    ATHCFG_WCMD_SET_DEV_CTSTIMEOUT,
    ATHCFG_WCMD_SET_DEV_SOFTLED,
    ATHCFG_WCMD_SET_DEV_LEDPIN,
    ATHCFG_WCMD_SET_DEV_DEBUG,
    ATHCFG_WCMD_SET_DEV_TXANTENNA,
    ATHCFG_WCMD_SET_DEV_RXANTENNA,
    ATHCFG_WCMD_SET_DEV_DIVERSITY,
    ATHCFG_WCMD_SET_DEV_TXINTRPERIOD,
    ATHCFG_WCMD_SET_DEV_FFTXQMIN,
    ATHCFG_WCMD_SET_DEV_TKIPMIC,
    ATHCFG_WCMD_SET_DEV_GLOBALTXTIMEOUT,
    ATHCFG_WCMD_SET_DEV_SW_WSC_BUTTON,
#ifdef OMNI_MX_LED
    ATHCFG_WCMD_SET_LEDSTATS, /*SAH MX LED set */
#endif
    ATHCFG_WCMD_SET_REGULATORYCODE,
    /* Device read specific */
    ATHCFG_WCMD_GET_DEV_EIFS_MASK,
    ATHCFG_WCMD_GET_DEV_EIFS_DUR,
    ATHCFG_WCMD_GET_DEV_SLOTTIME,
    ATHCFG_WCMD_GET_DEV_ACKTIMEOUT,
    ATHCFG_WCMD_GET_DEV_CTSTIMEOUT,
    ATHCFG_WCMD_GET_DEV_SOFTLED,
    ATHCFG_WCMD_GET_DEV_LEDPIN,
    ATHCFG_WCMD_GET_DEV_COUNTRYCODE,
    ATHCFG_WCMD_GET_DEV_REGDOMAIN,
    ATHCFG_WCMD_GET_DEV_DEBUG,
    ATHCFG_WCMD_GET_DEV_TXANTENNA,
    ATHCFG_WCMD_GET_DEV_RXANTENNA,
    ATHCFG_WCMD_GET_DEV_DIVERSITY,
    ATHCFG_WCMD_GET_DEV_TXINTRPERIOD,
    ATHCFG_WCMD_GET_DEV_FFTXQMIN,
    ATHCFG_WCMD_GET_DEV_TKIPMIC,
    ATHCFG_WCMD_GET_DEV_GLOBALTXTIMEOUT,
    ATHCFG_WCMD_GET_DEV_SW_WSC_BUTTON,
    ATHCFG_WCMD_GET_DEV_HST_STATS,
    ATHCFG_WCMD_GET_DEV_HST_11N_STATS,
    ATHCFG_WCMD_GET_DEV_TGT_STATS,
    ATHCFG_WCMD_GET_DEV_TGT_11N_STATS,
#ifdef OMNI_MX_LED
    ATHCFG_WCMD_GET_LEDSTATS,   /*SAH MX LED get*/
#endif
    ATHCFG_WCMD_NOMINAL_NF,
    ATHCFG_WCMD_MIN_NOISE_FLOOR,
    ATHCFG_WCMD_MAX_NOISE_FLOOR,
    ATHCFG_WCMD_NF_DELTA,
    ATHCFG_WCMD_NF_WEIGHT,
    ATHCFG_WCMD_CHANNEL_SWITCH,
    /* Antenna diversity commands */
    ATHCFG_WCMD_ENABLE_DIV,
    ATHCFG_WCMD_DISABLE_DIV,
    ATHCFG_WCMD_GET_DIV_PARAM,
    ATHCFG_WCMD_SET_DEF_TX,
    /* For debugging purpose*/
    ATHCFG_WCMD_SET_DBG_INFO,
    ATHCFG_WCMD_GET_DBG_INFO,
    ATHCFG_WCMD_GET_REG,
    ATHCFG_WCMD_SET_REG,
    /* Pktlog commands */
    ATHCFG_WCMD_ENABLE_PKTLOG,
    ATHCFG_WCMD_DISABLE_PKTLOG,
    ATHCFG_WCMD_PKTLOG_SET_SIZE,
    ATHCFG_WCMD_PKTLOG_GET_DATA,
    /* ether-dongle-mac command */
    ATHCFG_WCMD_GET_ETHER_DONGLE_MAC, 
    ATHCFG_WCMD_SET_ETHER_DONGLE_MAC, 
    ATHCFG_WCMD_SET_STA_ASSOC,
    ATHCFG_WCMD_GET_RADIO,     
    ATHCFG_WCMD_SET_RADIO,
    ATHCFG_WCMD_SET_SPEC_CHAN_SCAN,    
    ATHCFG_WCMD_GET_HIDDEN_SSID,
    ATHCFG_WCMD_SET_HIDDEN_SSID,
    ATHCFG_WCMD_GET_TESTMODE,
    ATHCFG_WCMD_SET_TESTMODE,
    ATHCFG_WCMD_ENABLE_TX99,
    ATHCFG_WCMD_DISABLE_TX99,
    ATHCFG_WCMD_SET_TX99_FREQ,
    ATHCFG_WCMD_SET_TX99_RATE,
    ATHCFG_WCMD_SET_TX99_POWER,
    ATHCFG_WCMD_SET_TX99_TXMODE,
    ATHCFG_WCMD_SET_TX99_TYPE,
    ATHCFG_WCMD_SET_TX99_CHANMASK,
    ATHCFG_WCMD_SET_TX99_RCVMODE,
    ATHCFG_WCMD_GET_TX99,
    ATHCFG_WCMD_GET_HTRATES,
    ATHCFG_WCMD_GET_WMI_STATS_TIMEOUT,
}athcfg_wcmd_type_t;
/**
 * @brief Opmodes for the VAP
 */
typedef enum athcfg_wcmd_opmode{
    ATHCFG_WCMD_OPMODE_IBSS,/**< IBSS (adhoc) station */
    ATHCFG_WCMD_OPMODE_STA,/**< Infrastructure station */
    ATHCFG_WCMD_OPMODE_WDS,/**< WDS link */
    ATHCFG_WCMD_OPMODE_AHDEMO,/**< Old lucent compatible adhoc demo */
    ATHCFG_WCMD_OPMODE_RESERVE0,/**<future use*/
    ATHCFG_WCMD_OPMODE_RESERVE1,/**<future use*/
    ATHCFG_WCMD_OPMODE_HOSTAP,/**< Software Access Point*/
    ATHCFG_WCMD_OPMODE_RESERVE2,/**<future use*/
    ATHCFG_WCMD_OPMODE_MONITOR/**< Monitor mode*/
}athcfg_wcmd_opmode_t;
/**
 * brief PHY modes for VAP
 */
typedef enum athcfg_wcmd_phymode{   
    ATHCFG_WCMD_PHYMODE_AUTO,/**< autoselect */
    ATHCFG_WCMD_PHYMODE_11A,/**< 5GHz, OFDM */
    ATHCFG_WCMD_PHYMODE_11B,/**< 2GHz, CCK */
    ATHCFG_WCMD_PHYMODE_11G,/**< 2GHz, OFDM */
    ATHCFG_WCMD_PHYMODE_FH,/**< 2GHz, GFSK */
    ATHCFG_WCMD_PHYMODE_TURBO_A,/**< 5GHz, OFDM, 2 x clk dynamic turbo */
    ATHCFG_WCMD_PHYMODE_TURBO_G,/**< 2GHz, OFDM, 2 x clk dynamic turbo*/
    ATHCFG_WCMD_PHYMODE_11NA,/**< 5GHz, OFDM + MIMO*/
    ATHCFG_WCMD_PHYMODE_11NG,/**< 2GHz, OFDM + MIMO*/
}athcfg_wcmd_phymode_t;
/**
 * @brief param id
 */
typedef enum athcfg_wcmd_param_id{
    ATHCFG_WCMD_PARAM_TURBO = 1,/**<turbo mode */
    ATHCFG_WCMD_PARAM_MODE,/**< phy mode (11a, 11b, etc.) */
    ATHCFG_WCMD_PARAM_AUTHMODE,/**< authentication mode */
    ATHCFG_WCMD_PARAM_PROTMODE,/**< 802.11g protection */
    ATHCFG_WCMD_PARAM_MCASTCIPHER,/**< multicast/default cipher */
    ATHCFG_WCMD_PARAM_MCASTKEYLEN,/**< multicast key length */
    ATHCFG_WCMD_PARAM_UCASTCIPHERS,/**< unicast cipher suites */
    ATHCFG_WCMD_PARAM_UCASTCIPHER,/**< unicast cipher */
    ATHCFG_WCMD_PARAM_UCASTKEYLEN,/**< unicast key length */
    ATHCFG_WCMD_PARAM_WPA,/**< WPA mode (0,1,2) */
    ATHCFG_WCMD_PARAM_ROAMING,/**< roaming mode */
    ATHCFG_WCMD_PARAM_PRIVACY,/**< privacy invoked */
    ATHCFG_WCMD_PARAM_COUNTERMEASURES,/**< WPA/TKIP countermeasures */
    ATHCFG_WCMD_PARAM_DROPUNENCRYPTED,/**< discard unencrypted frames */
    ATHCFG_WCMD_PARAM_DRIVER_CAPS,/**< driver capabilities */
    ATHCFG_WCMD_PARAM_MACCMD,/**< MAC ACL operation */
    ATHCFG_WCMD_PARAM_WMM,/**< WMM mode (on, off) */
    ATHCFG_WCMD_PARAM_HIDESSID,/**< hide SSID mode (on, off) */
    ATHCFG_WCMD_PARAM_APBRIDGE,/**< AP inter-sta bridging */
    ATHCFG_WCMD_PARAM_KEYMGTALGS,/**< key management algorithms */
    ATHCFG_WCMD_PARAM_RSNCAPS,/**< RSN capabilities */
    ATHCFG_WCMD_PARAM_INACT,/**< station inactivity timeout */
    ATHCFG_WCMD_PARAM_INACT_AUTH,/**< station auth inact timeout */
    ATHCFG_WCMD_PARAM_INACT_INIT,/**< station init inact timeout */
    ATHCFG_WCMD_PARAM_ABOLT,/**< Atheros Adv. Capabilities */
    ATHCFG_WCMD_PARAM_DTIM_PERIOD,/**< DTIM period (beacons) */
    ATHCFG_WCMD_PARAM_BEACON_INTERVAL,/**< beacon interval (ms) */
    ATHCFG_WCMD_PARAM_DOTH,/**< 11.h is on/off */
    ATHCFG_WCMD_PARAM_PWRTARGET,/**< Current Channel Pwr Constraint */
    ATHCFG_WCMD_PARAM_GENREASSOC,/**< Generate a reassociation request */
    ATHCFG_WCMD_PARAM_COMPRESSION,/**< compression */
    ATHCFG_WCMD_PARAM_FF,/**< fast frames support  */
    ATHCFG_WCMD_PARAM_XR,/**< XR support */
    ATHCFG_WCMD_PARAM_BURST,/**< burst mode */
    ATHCFG_WCMD_PARAM_PUREG,/**< pure 11g (no 11b stations) */
    ATHCFG_WCMD_PARAM_AR,/**< AR support */
    ATHCFG_WCMD_PARAM_WDS,/**< Enable 4 address processing */
    ATHCFG_WCMD_PARAM_BGSCAN,/**< bg scanning (on, off) */
    ATHCFG_WCMD_PARAM_BGSCAN_IDLE,/**< bg scan idle threshold */
    ATHCFG_WCMD_PARAM_BGSCAN_INTERVAL,/**< bg scan interval */
    ATHCFG_WCMD_PARAM_MCAST_RATE,/**< Multicast Tx Rate */
    ATHCFG_WCMD_PARAM_COVERAGE_CLASS,/**< coverage class */
    ATHCFG_WCMD_PARAM_COUNTRY_IE,/**< enable country IE */
    ATHCFG_WCMD_PARAM_SCANVALID,/**< scan cache valid threshold */
    ATHCFG_WCMD_PARAM_ROAM_RSSI_11A,/**< rssi threshold in 11a */
    ATHCFG_WCMD_PARAM_ROAM_RSSI_11B,/**< rssi threshold in 11b */
    ATHCFG_WCMD_PARAM_ROAM_RSSI_11G,/**< rssi threshold in 11g */
    ATHCFG_WCMD_PARAM_ROAM_RATE_11A,/**< tx rate threshold in 11a */
    ATHCFG_WCMD_PARAM_ROAM_RATE_11B,/**< tx rate threshold in 11b */
    ATHCFG_WCMD_PARAM_ROAM_RATE_11G,/**< tx rate threshold in 11g */
    ATHCFG_WCMD_PARAM_UAPSDINFO,/**< value for qos info field */
    ATHCFG_WCMD_PARAM_SLEEP,/**< force sleep/wake */
    ATHCFG_WCMD_PARAM_QOSNULL,/**< force sleep/wake */
    ATHCFG_WCMD_PARAM_PSPOLL,/**< force ps-poll generation (sta only) */
    ATHCFG_WCMD_PARAM_EOSPDROP,/**< force uapsd EOSP drop (ap only) */
    ATHCFG_WCMD_PARAM_MARKDFS,/**< mark a dfs interference channel*/
    ATHCFG_WCMD_PARAM_REGCLASS,/**< enable regclass ids in country IE */
    ATHCFG_WCMD_PARAM_CHANBW,/**< set chan bandwidth preference */
    ATHCFG_WCMD_PARAM_WMM_AGGRMODE,/**< set WMM Aggressive Mode */
    ATHCFG_WCMD_PARAM_SHORTPREAMBLE,/**< enable/disable short Preamble */
    ATHCFG_WCMD_PARAM_BLOCKDFSCHAN,/**< enable/disable use of DFS channels */
    ATHCFG_WCMD_PARAM_CWM_MODE,/**< CWM mode */
    ATHCFG_WCMD_PARAM_CWM_EXTOFFSET,/**< Ext. channel offset */
    ATHCFG_WCMD_PARAM_CWM_EXTPROTMODE,/**< Ext. Chan Protection mode */
    ATHCFG_WCMD_PARAM_CWM_EXTPROTSPACING,/**< Ext. chan Protection spacing */
    ATHCFG_WCMD_PARAM_CWM_ENABLE,/**< State machine enabled */
    ATHCFG_WCMD_PARAM_CWM_EXTBUSYTHRESHOLD,/**< Ext. chan busy threshold*/
    ATHCFG_WCMD_PARAM_CWM_CHWIDTH,/**< Current channel width */
    ATHCFG_WCMD_PARAM_SHORT_GI,/**< half GI */
    ATHCFG_WCMD_PARAM_FAST_CC,/**< fast channel change */
    /**
     * 11n A-MPDU, A-MSDU support
     */ 
    ATHCFG_WCMD_PARAM_AMPDU,/**< 11n a-mpdu support */
    ATHCFG_WCMD_PARAM_AMPDU_LIMIT,/**< a-mpdu length limit */
    ATHCFG_WCMD_PARAM_AMPDU_DENSITY,/**< a-mpdu density */
    ATHCFG_WCMD_PARAM_AMPDU_SUBFRAMES,/**< a-mpdu subframe limit */
    ATHCFG_WCMD_PARAM_AMSDU,/**< a-msdu support */
    ATHCFG_WCMD_PARAM_AMSDU_LIMIT,/**< a-msdu length limit */
    ATHCFG_WCMD_PARAM_COUNTRYCODE,/**< Get country code */
    ATHCFG_WCMD_PARAM_TX_CHAINMASK,/**< Tx chain mask */
    ATHCFG_WCMD_PARAM_RX_CHAINMASK,/**< Rx chain mask */
    ATHCFG_WCMD_PARAM_RTSCTS_RATECODE,/**< RTS Rate code */
    ATHCFG_WCMD_PARAM_HT_PROTECTION,/**< Protect traffic in HT mode */
    ATHCFG_WCMD_PARAM_RESET_ONCE,/**< Force a reset */
    ATHCFG_WCMD_PARAM_SETADDBAOPER,/**< Set ADDBA mode */
    ATHCFG_WCMD_PARAM_TX_CHAINMASK_LEGACY,/**< Tx chain mask */
    ATHCFG_WCMD_PARAM_11N_RATE,/**< Set ADDBA mode */
    ATHCFG_WCMD_PARAM_11N_RETRIES,/**< Tx chain mask for legacy clients */
    ATHCFG_WCMD_PARAM_WDS_AUTODETECT,/**< Autodetect/DelBa for WDS mode */
    ATHCFG_WCMD_PARAM_RB,/**< Switch in/out of RB */
    /**
     * RB Detection knobs.
     */ 
    ATHCFG_WCMD_PARAM_RB_DETECT,/**< Do RB detection */
    ATHCFG_WCMD_PARAM_RB_SKIP_THRESHOLD,/**< seqno-skip-by-1s to detect */
    ATHCFG_WCMD_PARAM_RB_TIMEOUT,/**< (in ms) to restore non-RB */
    ATHCFG_WCMD_PARAM_NO_HTIE,/**< Control HT IEs are sent out or parsed */
    ATHCFG_WCMD_PARAM_MAXSTA,/**< Config max allowable staions for each VAP */
    ATHCFG_WCMD_PARAM_ETHER_DONGLE, /**< Station dongle mode */
    /* TX99 */
	ATHCFG_WCMD_PARAM_TX99
}athcfg_wcmd_param_id_t;
/**
 *  @brief APPIEBUF related definations
 */
typedef enum athcfg_wcmd_appie_frame{
    ATHCFG_WCMD_APPIE_FRAME_BEACON,
    ATHCFG_WCMD_APPIE_FRAME_PROBE_REQ,
    ATHCFG_WCMD_APPIE_FRAME_PROBE_RESP,
    ATHCFG_WCMD_APPIE_FRAME_ASSOC_REQ,
    ATHCFG_WCMD_APPIE_FRAME_ASSOC_RESP,
    ATHCFG_WCMD_APPIE_NUM_OF_FRAME
}athcfg_wcmd_appie_frame_t;
/**
 * @brief filter pointer info
 */
typedef enum athcfg_wcmd_filter_type{
    ATHCFG_WCMD_FILTER_TYPE_BEACON=0x1,
    ATHCFG_WCMD_FILTER_TYPE_PROBE_REQ=0x2,
    ATHCFG_WCMD_FILTER_TYPE_PROBE_RESP=0x4,
    ATHCFG_WCMD_FILTER_TYPE_ASSOC_REQ=0x8,
    ATHCFG_WCMD_FILTER_TYPE_ASSOC_RESP=0x10,
    ATHCFG_WCMD_FILTER_TYPE_AUTH=0x20,
    ATHCFG_WCMD_FILTER_TYPE_DEAUTH=0x40,
    ATHCFG_WCMD_FILTER_TYPE_DISASSOC=0x80,
    ATHCFG_WCMD_FILTER_TYPE_ALL=0xFF,
}athcfg_wcmd_filter_type_t;
/**
 * @brief mlme info pointer
 */
typedef enum athcfg_wcmd_mlme_op_type{
    ATHCFG_WCMD_MLME_ASSOC,
    ATHCFG_WCMD_MLME_DISASSOC,
    ATHCFG_WCMD_MLME_DEAUTH,
    ATHCFG_WCMD_MLME_AUTHORIZE,
    ATHCFG_WCMD_MLME_UNAUTHORIZE,
}athcfg_wcmd_mlme_op_type_t;

typedef enum athcfg_wcmd_wmmparams{
    ATHCFG_WCMD_WMMPARAMS_CWMIN = 1,
    ATHCFG_WCMD_WMMPARAMS_CWMAX,
    ATHCFG_WCMD_WMMPARAMS_AIFS,
    ATHCFG_WCMD_WMMPARAMS_TXOPLIMIT,
    ATHCFG_WCMD_WMMPARAMS_ACM,
    ATHCFG_WCMD_WMMPARAMS_NOACKPOLICY, 
}athcfg_wcmd_wmmparams_t;
/**
 * @brief Power Management Flags
 */
typedef enum athcfg_wcmd_pmflags{
    ATHCFG_WCMD_POWER_ON  = 0x0,
    ATHCFG_WCMD_POWER_MIN = 0x1,/**< Min */
    ATHCFG_WCMD_POWER_MAX = 0x2,/**< Max */
    ATHCFG_WCMD_POWER_REL = 0x4,/**< Not in seconds/ms/us */
    ATHCFG_WCMD_POWER_MOD = 0xF,/**< Modify a parameter */
    ATHCFG_WCMD_POWER_UCAST_R = 0x100,/**< Ucast messages */
    ATHCFG_WCMD_POWER_MCAST_R = 0x200,/**< Mcast messages */
    ATHCFG_WCMD_POWER_ALL_R = 0x300,/**< All messages though PM */
    ATHCFG_WCMD_POWER_FORCE_S = 0x400,/**< Force PM to unicast */
    ATHCFG_WCMD_POWER_REPEATER = 0x800,/**< Bcast messages in PM*/
    ATHCFG_WCMD_POWER_MODE = 0xF00,/**< Power Management mode */
    ATHCFG_WCMD_POWER_PERIOD = 0x1000,/**< Period/Duration of */
    ATHCFG_WCMD_POWER_TIMEOUT = 0x2000,/**< Timeout (to go asleep) */
    ATHCFG_WCMD_POWER_TYPE = 0xF000/**< Type of parameter */
}athcfg_wcmd_pmflags_t;
/**
 * @brief Tx Power flags
 */
typedef enum athcfg_wcmd_txpow_flags{
    ATHCFG_WCMD_TXPOW_DBM = 0,/**< dBm */
    ATHCFG_WCMD_TXPOW_MWATT = 0x1,/**< mW */
    ATHCFG_WCMD_TXPOW_RELATIVE = 0x2,/**< Arbitrary units */ 
    ATHCFG_WCMD_TXPOW_DBM_EX = 0x3,
    ATHCFG_WCMD_TXPOW_TYPE = 0xFF,/**< Type of value */        
    ATHCFG_WCMD_TXPOW_RANGE = 0x1000/**< Range (min - max) */ 
}athcfg_wcmd_txpow_flags_t;
/**
 * @brief Retry flags
 */
typedef enum athcfg_wcmd_retry_flags{
    ATHCFG_WCMD_RETRY_ON = 0x0,
    ATHCFG_WCMD_RETRY_MIN = 0x1,/**< Value is a minimum */
    ATHCFG_WCMD_RETRY_MAX = 0x2,/**< Maximum */
    ATHCFG_WCMD_RETRY_RELATIVE = 0x4,/**< Not in seconds/ms/us */
    ATHCFG_WCMD_RETRY_SHORT = 0x10,/**< Short packets  */
    ATHCFG_WCMD_RETRY_LONG = 0x20,/**< Long packets */ 
    ATHCFG_WCMD_RETRY_MODIFIER = 0xFF,/**< Modify a parameter */
    ATHCFG_WCMD_RETRY_LIMIT = 0x1000,/**< Max retries*/
    ATHCFG_WCMD_RETRY_LIFETIME = 0x2000,/**< Max retries us*/
    ATHCFG_WCMD_RETRY_TYPE = 0xF000/**< Parameter type */
} athcfg_wcmd_retry_flags_t;

/**
 * @brief CWM Debug mode commands
 */
typedef enum athcfg_wcmd_cwm_cmd{
    ATHCFG_WCMD_CWM_CMD_EVENT,/**< Send Event */
    ATHCFG_WCMD_CWM_CMD_CTL,/**< Ctrl channel busy */
    ATHCFG_WCMD_CWM_CMD_EXT,/**< Ext chan busy */
    ATHCFG_WCMD_CWM_CMD_VCTL,/**< virt ctrl chan busy*/
    ATHCFG_WCMD_CWM_CMD_VEXT/**< virt extension channel busy*/
}athcfg_wcmd_cwm_cmd_t;
/**
 * @brief CWM EVENTS
 */
typedef enum athcfg_wcmd_cwm_event{
    ATHCFG_WCMD_CWMEVENT_TXTIMEOUT,  /**< tx timeout interrupt */
    ATHCFG_WCMD_CWMEVENT_EXTCHCLEAR, /**< ext channel sensing clear */
    ATHCFG_WCMD_CWMEVENT_EXTCHBUSY,  /**< ext channel sensing busy */
    ATHCFG_WCMD_CWMEVENT_EXTCHSTOP,  /**< ext channel sensing stop */
    ATHCFG_WCMD_CWMEVENT_EXTCHRESUME,/**< ext channel sensing resume */
    ATHCFG_WCMD_CWMEVENT_DESTCW20,   /**< dest channel width changed to 20 */
    ATHCFG_WCMD_CWMEVENT_DESTCW40,   /**< dest channel width changed to 40 */
    ATHCFG_WCMD_CWMEVENT_MAX 
} athcfg_wcmd_cwm_event_t;
/**
 * @brief eth tool info  
 */
typedef enum athcfg_wcmd_ethtool_cmd{
    ATHCFG_WCMD_ETHTOOL_GSET=0x1,/**< Get settings. */
    ATHCFG_WCMD_ETHTOOL_SSET,/**< Set settings. */
    ATHCFG_WCMD_ETHTOOL_GDRVINFO,/**< Get driver info. */
    ATHCFG_WCMD_ETHTOOL_GREGS,/**< Get NIC registers. */
    ATHCFG_WCMD_ETHTOOL_GWOL,/**< Wake-on-lan options. */
    ATHCFG_WCMD_ETHTOOL_SWOL,/**< Set wake-on-lan options. */
    ATHCFG_WCMD_ETHTOOL_GMSGLVL,/**< Get driver message level */
    ATHCFG_WCMD_ETHTOOL_SMSGLVL,/**< Set driver msg level */
    ATHCFG_WCMD_ETHTOOL_NWAY_RST,/**< Restart autonegotiation. */ 
    ATHCFG_WCMD_ETHTOOL_GEEPROM,/**< Get EEPROM data */
    ATHCFG_WCMD_ETHTOOL_SEEPROM,/** < Set EEPROM data. */
    ATHCFG_WCMD_ETHTOOL_GCOALESCE,/** < Get coalesce config */
    ATHCFG_WCMD_ETHTOOL_SCOALESCE,/** < Set coalesce config. */
    ATHCFG_WCMD_ETHTOOL_GRINGPARAM,/** < Get ring parameters */
    ATHCFG_WCMD_ETHTOOL_SRINGPARAM,/** < Set ring parameters. */
    ATHCFG_WCMD_ETHTOOL_GPAUSEPARAM,/** < Get pause parameters */
    ATHCFG_WCMD_ETHTOOL_SPAUSEPARAM,/** < Set pause parameters. */
    ATHCFG_WCMD_ETHTOOL_GRXCSUM,/** < Get RX hw csum enable */
    ATHCFG_WCMD_ETHTOOL_SRXCSUM,/** < Set RX hw csum enable */
    ATHCFG_WCMD_ETHTOOL_GTXCSUM,/** < Get TX hw csum enable */
    ATHCFG_WCMD_ETHTOOL_STXCSUM,/** < Set TX hw csum enable */
    ATHCFG_WCMD_ETHTOOL_GSG,/** < Get scatter-gather enable */
    ATHCFG_WCMD_ETHTOOL_SSG,/** < Set scatter-gather enable */
    ATHCFG_WCMD_ETHTOOL_TEST,/** < execute NIC self-test. */
    ATHCFG_WCMD_ETHTOOL_GSTRINGS,/** < get specified string set */
    ATHCFG_WCMD_ETHTOOL_PHYS_ID,/** < identify the NIC */
    ATHCFG_WCMD_ETHTOOL_GSTATS,/** < get NIC-specific statistics */
    ATHCFG_WCMD_ETHTOOL_GTSO,/** < Get TSO enable (ethtool_value) */
    ATHCFG_WCMD_ETHTOOL_STSO,/** < Set TSO enable (ethtool_value) */
    ATHCFG_WCMD_ETHTOOL_GPERMADDR,/** < Get permanent hardware address */
    ATHCFG_WCMD_ETHTOOL_GUFO,/** < Get UFO enable */
    ATHCFG_WCMD_ETHTOOL_SUFO,/** < Set UFO enable */
    ATHCFG_WCMD_ETHTOOL_GGSO,/** < Get GSO enable */
    ATHCFG_WCMD_ETHTOOL_SGSO,/** < Set GSO enable */
}athcfg_wcmd_ethtool_cmd_t;

/*
 * Reason codes
 *
 * Unlisted codes are reserved
 */
enum {
    ATHCFG_WCMD_REASON_UNSPECIFIED        = 1,
    ATHCFG_WCMD_REASON_AUTH_EXPIRE        = 2,
    ATHCFG_WCMD_REASON_AUTH_LEAVE     = 3,
    ATHCFG_WCMD_REASON_ASSOC_EXPIRE       = 4,
    ATHCFG_WCMD_REASON_ASSOC_TOOMANY      = 5,
    ATHCFG_WCMD_REASON_NOT_AUTHED     = 6,
    ATHCFG_WCMD_REASON_NOT_ASSOCED        = 7,
    ATHCFG_WCMD_REASON_ASSOC_LEAVE        = 8,
    ATHCFG_WCMD_REASON_ASSOC_NOT_AUTHED   = 9,

    ATHCFG_WCMD_REASON_RSN_REQUIRED       = 11,
    ATHCFG_WCMD_REASON_RSN_INCONSISTENT   = 12,
    ATHCFG_WCMD_REASON_IE_INVALID     = 13,
    ATHCFG_WCMD_REASON_MIC_FAILURE        = 14,

    ATHCFG_WCMD_STATUS_SUCCESS        = 0,
    ATHCFG_WCMD_STATUS_UNSPECIFIED        = 1,
    ATHCFG_WCMD_STATUS_CAPINFO        = 10,
    ATHCFG_WCMD_STATUS_NOT_ASSOCED        = 11,
    ATHCFG_WCMD_STATUS_OTHER          = 12,
    ATHCFG_WCMD_STATUS_ALG            = 13,
    ATHCFG_WCMD_STATUS_SEQUENCE       = 14,
    ATHCFG_WCMD_STATUS_CHALLENGE      = 15,
    ATHCFG_WCMD_STATUS_TIMEOUT        = 16,
    ATHCFG_WCMD_STATUS_TOOMANY        = 17,
    ATHCFG_WCMD_STATUS_BASIC_RATE     = 18,
    ATHCFG_WCMD_STATUS_SP_REQUIRED        = 19,
    ATHCFG_WCMD_STATUS_PBCC_REQUIRED      = 20,
    ATHCFG_WCMD_STATUS_CA_REQUIRED        = 21,
    ATHCFG_WCMD_STATUS_TOO_MANY_STATIONS  = 22,
    ATHCFG_WCMD_STATUS_RATES          = 23,
    ATHCFG_WCMD_STATUS_SHORTSLOT_REQUIRED = 25,
    ATHCFG_WCMD_STATUS_DSSSOFDM_REQUIRED  = 26,
    ATHCFG_WCMD_STATUS_REFUSED        = 37,
    ATHCFG_WCMD_STATUS_INVALID_PARAM      = 38,
};

typedef enum athcfg_wcmd_dbg_type{
    ATHCFG_WCMD_DBG_DUMMY   = 0,
    ATHCFG_WCMD_DBG_TRIGGER = 1,
    ATHCFG_WCMD_DBG_CAP_PKT = 2,
    ATHCFG_WCMD_DBG_OTHER   = 3,
    ATHCFG_WCMD_DBG_MAX
}athcfg_wcmd_dbg_type_t;

/**
 * ***************************Structures***********************
 */
/**
 * @brief Mac Address info
 */ 
typedef struct athcfg_ethaddr{
        a_uint8_t   addr[ATHCFG_ETH_LEN];
} athcfg_ethaddr_t;                

/**
 * @brief Information Element
 */
typedef struct athcfg_ie_info{
    a_uint16_t         len;
    a_uint8_t          data[ATHCFG_WCMD_IE_MAXLEN];
}athcfg_ie_info_t;

/**
 * @brief WCMD info
 */
typedef struct athcfg_wcmd_vapname{
    a_uint32_t     len;
   a_uint8_t      name[ATHCFG_WCMD_NAME_SIZE];
}athcfg_wcmd_vapname_t;

/**
 * @brief nickname pointer info
 */
typedef struct athcfg_wcmd_nickname{
    a_uint16_t      len;
    a_uint8_t       name[ATHCFG_WCMD_NICK_NAME];
}athcfg_wcmd_nickname_t;

/**
 * @brief missed frame info
 */
typedef struct  athcfg_wcmd_miss{
    a_uint32_t      beacon;/**< Others cases */
}athcfg_wcmd_miss_t;

/**
 * @brief  discarded frame info
 */
typedef struct  athcfg_wcmd_discard{
    a_uint32_t      nwid;/**< Rx : Wrong nwid/essid */
    a_uint32_t      code; /**< Rx : Unable to code/decode (WEP) */
    a_uint32_t      fragment;/**< Rx : Can't perform MAC reassembly */
    a_uint32_t      retries;/**< Tx : Max MAC retries num reached */
    a_uint32_t      misc;/**< Others cases */
}athcfg_wcmd_discard_t;

/**
 * @brief Link quality info
 */
typedef struct  athcfg_wcmd_linkqty{
    a_uint8_t       qual;/*link quality(retries, SNR, missed beacons)*/ 
    a_uint8_t       level;/*Signal level (dBm) */
    a_uint8_t       noise;/*Noise level (dBm) */
    a_uint8_t       updated;/*Update flag*/
}athcfg_wcmd_linkqty_t;

/**
 * @brief frequency info
 */
typedef struct  athcfg_wcmd_freq{
    a_int32_t       m;/*Mantissa */
    a_int16_t       e;/*Exponent */
    a_uint8_t       i;/*List index (when in range struct) */
    a_uint8_t       flags;/*Flags (fixed/auto) */
}athcfg_wcmd_freq_t;

/**
 * @brief VAP parameter range info
 */
typedef struct athcfg_wcmd_vapparam_range{
    
    /**
     * @brief Informative stuff (to choose between different 
     * interface) In theory this value should be the maximum 
     * benchmarked TCP/IP throughput, because with most of these 
     * devices the bit rate is meaningless (overhead an co) to 
     * estimate how fast the connection will go and pick the fastest 
     * one. I suggest people to play with Netperf or any 
     * benchmark...
     */
    a_uint32_t           throughput;/**< To give an idea... */
    
    /** @brief NWID (or domain id) */
    a_uint32_t           min_nwid;/**< Min NWID to set */
    a_uint32_t           max_nwid;/**< Max NWID to set */

    /**
     * @brief Old Frequency (backward compatibility - moved lower )
     */
    a_uint16_t           old_num_channels;
    a_uint8_t            old_num_frequency;

    /** @brief Wireless event capability bitmasks */
    a_uint32_t          event_capa[ATHCFG_WCMD_EVENT_CAP];

    /** @brief Signal level threshold range */
    a_int32_t           sensitivity;

    /**
     * @brief Quality of link & SNR stuff Quality range (link,
     * level,noise) If the quality is absolute, it will be in the 
     * range [0
     * - max_qual], if the quality is dBm, it will be in the range
     * [max_qual - 0]. Don't forget that we use 8 bit arithmetics...
     */
    athcfg_wcmd_linkqty_t       max_qual;/**< Link Quality*/

    /**
     * @brief This should contain the average/typical values of the 
     * quality indicator. This should be the threshold between a 
     * "good" and a "bad" link (example : monitor going from green
     * to orange). Currently, user space apps like quality monitors
     * don't have any way to calibrate the measurement. With this,
     * they can split the range between 0 and max_qual in different
     * quality level (using a geometric subdivision centered on the
     * average). I expect that people doing the user space apps will
     * feedback us on which value we need to put in each
     * driver... 
     */
    athcfg_wcmd_linkqty_t            avg_qual; 

    /** @brief Rates */
    a_uint8_t           num_bitrates; /**< Number of entries in the list */
    a_int32_t           bitrate[ATHCFG_WCMD_MAX_BITRATES]; /**< in bps */

    /** @brief  RTS threshold */
    a_int32_t           min_rts; /**< Minimal RTS threshold */
    a_int32_t           max_rts; /**< Maximal RTS threshold */

    /** @brief  Frag threshold */
    a_int32_t           min_frag;/**< Minimal frag threshold */
    a_int32_t           max_frag;/**< Maximal frag threshold */

    /** @brief  Power Management duration & timeout */
    a_int32_t           min_pmp;/**< Minimal PM period */
    a_int32_t           max_pmp;/**<  Maximal PM period */
    a_int32_t           min_pmt;/**< Minimal PM timeout */
    a_int32_t           max_pmt;/**< Maximal PM timeout */
    a_uint16_t          pmp_flags;/**< decode max/min PM period */
    a_uint16_t          pmt_flags;/**< decode max/min PM timeout */
    a_uint16_t          pm_capa;/**< PM options supported */

    /** @brief  Encoder stuff, Different token sizes */
    a_uint16_t          enc_sz[ATHCFG_WCMD_MAX_ENC_SZ];
    a_uint8_t           num_enc_sz; /**< Number of entry in the list */
    a_uint8_t           max_enc_tk;/**< Max number of tokens */
    /** @brief  For drivers that need a "login/passwd" form */
    a_uint8_t           enc_login_idx;/**< token index for login token */

    /** @brief  Transmit power */
    a_uint16_t          txpower_capa;/**< options supported */
    a_uint8_t           num_txpower;/**< Number of entries in the list */
    a_int32_t           txpower[ATHCFG_WCMD_MAX_TXPOWER];/**< in bps */
    
    /** @brief  Wireless Extension version info */
    a_uint8_t           we_version_compiled;/**< Must be WIRELESS_EXT */
    a_uint8_t           we_version_source;/**< Last update of source */
    
    /** @brief  Retry limits and lifetime */
    a_uint16_t          retry_capa;/**< retry options supported */
    a_uint16_t          retry_flags;/**< decode max/min retry limit*/
    a_uint16_t          r_time_flags;/**< Decode max/min retry life */
    a_int32_t           min_retry;/**< Min retries */
    a_int32_t           max_retry;/**< Max retries */
    a_int32_t           min_r_time;/**< Min retry lifetime */
    a_int32_t           max_r_time;/**< Max retry lifetime */
    
    /** @brief Frequency */
    a_uint16_t          num_channels;/**< Num channels [0 - (num - 1)] */
    a_uint8_t           num_frequency;/**< Num entries*/
    /**
     * Note : this frequency list doesn't need to fit channel
     * numbers, because each entry contain its channel index
     */
    athcfg_wcmd_freq_t    freq[ATHCFG_WCMD_MAX_FREQ];
    
    a_uint32_t          enc_capa; /**< IW_ENC_CAPA_* bit field */
}athcfg_wcmd_vapparam_range_t;

/**
 * @brief key info
 */
typedef struct athcfg_wcmd_keyinfo{
    a_uint8_t   ik_type; /**< key/cipher type */
    a_uint8_t   ik_pad;
    a_uint16_t  ik_keyix;/**< key index */
    a_uint8_t   ik_keylen;/**< key length in bytes */
    a_uint32_t   ik_flags;
    a_uint8_t   ik_macaddr[ATHCFG_WCMD_ADDR_LEN];
    a_uint64_t  ik_keyrsc;/**< key receive sequence counter */
    a_uint64_t  ik_keytsc;/**< key transmit sequence counter */
    a_uint8_t   ik_keydata[ATHCFG_WCMD_KEYDATA_SZ];
}athcfg_wcmd_keyinfo_t;

/**
 * @brief bssid pointer info
 */
typedef struct athcfg_wcmd_bssid{
    a_uint8_t      bssid[ATHCFG_WCMD_ADDR_LEN];
}athcfg_wcmd_bssid_t;

/**
 * @brief essid structure info
 */
typedef struct  athcfg_wcmd_ssid{
    a_uint8_t     byte[ATHCFG_WCMD_MAX_SSID];
    a_uint16_t    len;/**< number of fields or size in bytes */
    a_uint16_t    flags;/**< Optional  */
} athcfg_wcmd_ssid_t;

typedef struct  athcfg_wcmd_testmode{
    a_uint8_t     bssid[ATHCFG_WCMD_ADDR_LEN];
    a_int32_t     freq;/*MHz */
    a_uint16_t    mode;/* mode */
    a_int32_t     rssi_combined;/* RSSI */
    a_int32_t     rssi0;/* RSSI */
    a_int32_t     rssi1;/* RSSI */
    a_int32_t     rssi2;/* RSSI */
} athcfg_wcmd_testmode_t;

typedef struct athcfg_wcmd_param{
    athcfg_wcmd_param_id_t param_id;
    a_uint32_t value;
}athcfg_wcmd_param_t;

/**
 * @brief optional IE pointer info
 */
typedef athcfg_ie_info_t  athcfg_wcmd_optie_t;

/**
 * @brief status of VAP interface 
 */ 
typedef struct athcfg_wcmd_vapstats{
    a_uint8_t                  status;/**< Status*/
    athcfg_wcmd_linkqty_t     qual;/**< Quality of the link*/
    athcfg_wcmd_discard_t     discard;/**< Packet discarded counts */ 
    athcfg_wcmd_miss_t      miss;/**< Packet missed counts */
} athcfg_wcmd_vapstats_t;

/**
 * @brief appie pointer info
 */
typedef struct athcfg_wcmd_appie{
    athcfg_wcmd_appie_frame_t  frmtype;
    a_uint32_t                 len;
    a_uint8_t                  data[ATHCFG_WCMD_IE_MAXLEN];
}athcfg_wcmd_appie_t;

/**
 * @brief send ADDBA info pointer
 */
typedef struct athcfg_wcmd_addba{
    a_uint16_t  aid;
    a_uint32_t  tid;
    a_uint32_t  size;
}athcfg_wcmd_addba_t;

/**
 * @brief ADDBA status pointer info
 */
typedef struct athcfg_wcmd_addba_status{
    a_uint16_t  aid;
    a_uint32_t  tid;
    a_uint16_t  status;
}athcfg_wcmd_addba_status_t;

/**
 * @brief ADDBA response pointer info
 */
typedef struct athcfg_wcmd_addba_resp{
    a_uint16_t  aid;
    a_uint32_t  tid;
    a_uint32_t  reason;
}athcfg_wcmd_addba_resp_t;

/**
 * @brief send DELBA info pointer
 */
typedef struct athcfg_wcmd_delba{
    a_uint16_t  aid;
    a_uint32_t  tid;
    a_uint32_t  identifier;
    a_uint32_t  reason;
}athcfg_wcmd_delba_t;

/**
 * @brief MLME
 */
typedef struct athcfg_wcmd_mlme{
    athcfg_wcmd_mlme_op_type_t  op;/**< operation to perform */ 
    a_uint8_t                    reason;/**< 802.11 reason code */
    //a_uint8_t                  macaddr[ADF_NET_WCMD_ADDR_LEN];
    athcfg_ethaddr_t            mac;
}athcfg_wcmd_mlme_t;

/**
 * @brief Set the active channel list.  Note this list is
 * intersected with the available channel list in
 * calculating the set of channels actually used in
 * scanning.
 */
typedef struct athcfg_wcmd_chanlist{
    a_uint8_t   chanlist[ATHCFG_WCMD_CHAN_BYTES];
//    u_int16_t   len;
}athcfg_wcmd_chanlist_t;

/**
 * @brief Channels are specified by frequency and attributes.
 */
typedef struct athcfg_wcmd_chan{
    a_uint16_t  freq;/**< setting in Mhz */
    a_uint32_t  flags;/**< see below */
    a_uint8_t   ieee;/**< IEEE channel number */
    a_int8_t    maxregpower;/**< maximum regulatory tx power in dBm */
    a_int8_t    maxpower;/**< maximum tx power in dBm */
    a_int8_t    minpower;/**< minimum tx power in dBm */
    a_uint8_t   regclassid;/**< regulatory class id */
} athcfg_wcmd_chan_t;

/**
 * @brief channel info pointer
 */
typedef struct athcfg_wcmd_chaninfo{
    a_uint32_t  nchans;
    athcfg_wcmd_chan_t chans;
}athcfg_wcmd_chaninfo_t;

/**
 * @brief wmm-param info 
 */ 
typedef struct athcfg_wcmd_wmmparaminfo{
    athcfg_wcmd_wmmparams_t cmd;
    a_uint32_t  ac;
    a_uint32_t  bss;
    a_uint32_t  value;
}athcfg_wcmd_wmmparaminfo_t;

/**
 * @brief wpa ie pointer info
 */
typedef struct athcfg_wcmd_wpaie{
    athcfg_ethaddr_t  mac;
    athcfg_ie_info_t  ie;
//wpa2port
    athcfg_ie_info_t  wps_ie;
}athcfg_wcmd_wpaie_t;

/**
 * @brief wsc ie pointer info
 */
typedef struct athcfg_wcmd_wscie{
    athcfg_ethaddr_t   mac;
    athcfg_ie_info_t   ie;
}athcfg_wcmd_wscie_t;

/**
 * @brief rts threshold set/get info
 */
typedef struct athcfg_wcmd_rts_th{
    a_uint16_t          threshold;
    a_uint16_t          disabled;
    a_uint16_t          fixed;
}athcfg_wcmd_rts_th_t;

/**
 * @brief fragment threshold set/get info
 */
typedef struct athcfg_wcmd_frag_th{
    a_uint16_t     threshold;
    a_uint16_t     disabled;
    a_uint16_t     fixed;
}athcfg_wcmd_frag_th_t;

/**
 * @brief ic_caps set/get/enable/disable info
 */
typedef a_uint32_t     athcfg_wcmd_ic_caps_t;

/**
 * @brief retries set/get/enable/disable info
 */
typedef struct athcfg_wcmd_retries{
  a_int32_t          value;/**< The value of the parameter itself */
  a_uint8_t          disabled;/**< Disable the feature */
  a_uint16_t         flags;/**< Various specifc flags (if any) */
}athcfg_wcmd_retries_t;

/**
 * @brief power set/get info
 */
typedef struct athcfg_wcmd_power{
  a_int32_t               value;/**< The value of the parameter itself */
  a_uint8_t               disabled;/**< Disable the feature */
  athcfg_wcmd_pmflags_t  flags;/**< Various specifc flags (if any) */
  a_int32_t               fixed;/**< fixed */
}athcfg_wcmd_power_t;

/**
 * @brief txpower set/get/enable/disable info
 */
typedef struct athcfg_wcmd_txpower{
    a_uint32_t                     txpower;
    a_uint8_t                      fixed;
    a_uint8_t                      disabled;
    athcfg_wcmd_txpow_flags_t     flags;
    a_uint32_t                     freq;
    a_uint8_t                      txpowertable[ATHCFG_WCMD_TX_POWER_TABLE_SIZE];
}athcfg_wcmd_txpower_t;

/**
 * @brief tx-power-limit info 
 */ 
typedef a_uint32_t  athcfg_wcmd_txpowlimit_t;

/**
 * @brief Scan result data returned
 */
typedef struct athcfg_wcmd_scan_result{
    a_uint16_t  isr_len;        /**< length (mult of 4) */
    a_uint16_t  isr_freq;       /**< MHz */
    a_uint32_t  isr_flags;     /**< channel flags */
    a_uint8_t   isr_noise;
    a_uint8_t   isr_rssi;
    a_uint8_t   isr_intval;     /**< beacon interval */
    a_uint16_t  isr_capinfo;    /**< capabilities */
    a_uint8_t   isr_erp;        /**< ERP element */
    a_uint8_t   isr_bssid[ATHCFG_WCMD_ADDR_LEN];
    a_uint8_t   isr_nrates;
    a_uint8_t   isr_rates[ATHCFG_WCMD_RATE_MAXSIZE];
    a_uint8_t   isr_ssid_len;   /**< SSID length */
    a_uint8_t   isr_ie_len;     /**< IE length */
    a_uint8_t   isr_pad[5];
    /* variable length SSID */
    a_uint8_t   isr_ssid[ATHCFG_WCMD_MAX_SSID];
    /* IE data */
    athcfg_ie_info_t   isr_wpa_ie;
    athcfg_ie_info_t   isr_wme_ie;
    athcfg_ie_info_t   isr_ath_ie;
    athcfg_ie_info_t   isr_rsn_ie;
    athcfg_ie_info_t   isr_wps_ie;
    athcfg_ie_info_t   isr_htcap_ie;
    athcfg_ie_info_t   isr_htinfo_ie;
    a_uint8_t          isr_htcap_mcsset[ATHCFG_WCMD_MAX_HT_MCSSET];
} athcfg_wcmd_scan_result_t;

/**
 * @brief scan request info
 */
typedef struct athcfg_wcmd_scan{
    athcfg_wcmd_scan_result_t   result[ATHCFG_WCMD_MAX_AP];
    a_uint32_t                  len;/*Valid entries*/
    a_uint8_t                   more;
    a_uint8_t                   offset;
}athcfg_wcmd_scan_t;

/**
 * waplist data structure
 */
typedef struct athcfg_vaplist {
    athcfg_ethaddr_t        mac;
    athcfg_wcmd_opmode_t    mode;
    athcfg_wcmd_linkqty_t   qual;
} athcfg_vaplist_t;        


/**
 * @brief waplist request info
 */
typedef struct athcfg_wcmd_vaplist{
    athcfg_vaplist_t        list[ATHCFG_WCMD_MAX_AP];
    a_uint32_t              len;   
}athcfg_wcmd_vaplist_t;

/**
 * @brief Station information block; the mac address is used
 * to retrieve other data like stats, unicast key, etc.
 */
typedef struct athcfg_wcmd_sta{
    a_uint16_t  isi_len;/**< length (mult of 4) */
    a_uint16_t  isi_freq;/**< MHz */
    a_uint32_t  isi_flags;/**< channel flags */
    a_uint16_t  isi_state;/**< state flags */
    a_uint8_t   isi_authmode;/**< authentication algorithm */
    a_int8_t    isi_rssi;
    a_uint16_t  isi_capinfo;/**< capabilities */
    a_uint8_t   isi_athflags;/**< Atheros capabilities */
    a_uint8_t   isi_erp;/**< ERP element */
    a_uint8_t   isi_macaddr[ATHCFG_WCMD_ADDR_LEN];
    a_uint8_t   isi_nrates;/**< negotiated rates */
    a_uint8_t   isi_rates[ATHCFG_WCMD_RATE_MAXSIZE];
    a_uint8_t   isi_txrate;/**< index to isi_rates[] */
    a_uint32_t  isi_txrateKbps;/**< rate in Kbps , for 11n */
    a_uint16_t  isi_ie_len;/**< IE length */
    a_uint16_t  isi_associd;/**< assoc response */
    a_uint16_t  isi_txpower;/**< current tx power */
    a_uint16_t  isi_vlan;/**< vlan tag */
    a_uint16_t  isi_txseqs[17];/**< seq to be transmitted */
    a_uint16_t  isi_rxseqs[17];/**< seq previous for qos frames*/
    a_uint16_t  isi_inact;/**< inactivity timer */
    a_uint8_t   isi_uapsd;/**< UAPSD queues */
    a_uint8_t   isi_opmode;/**< sta operating mode */
    a_uint8_t   isi_cipher;
    a_uint32_t  isi_assoc_time;/**< sta association time */
    a_uint16_t  isi_htcap;/**< HT capabilities */
    /* variable length IE data */
    a_uint8_t   isi_wpa_ie[ATHCFG_WCMD_IE_MAXLEN];
    a_uint8_t   isi_wme_ie[ATHCFG_WCMD_IE_MAXLEN];
    a_uint8_t   isi_ath_ie[ATHCFG_WCMD_IE_MAXLEN]; 
}athcfg_wcmd_sta_t;

/**
 * @brief list of stations
 */
typedef struct athcfg_wcmd_stainfo{
    athcfg_wcmd_sta_t  list[ATHCFG_WCMD_MAX_AP];
    a_uint32_t         len;
} athcfg_wcmd_stainfo_t;

/**
 * @brief ath caps info
 */
typedef struct athcfg_wcmd_devcap{ 
    a_int32_t   cap; 
    a_int32_t   setting; 
}athcfg_wcmd_devcap_t; 

/**
 * @brief station stats
 */
typedef struct athcfg_wcmd_stastats{
    athcfg_ethaddr_t  mac;/**< MAC of the station*/
    
    struct ns_data{
        a_uint32_t  ns_rx_data;/**< rx data frames */
        a_uint32_t  ns_rx_mgmt;/**< rx management frames */
        a_uint32_t  ns_rx_ctrl;/**< rx control frames */
        a_uint32_t  ns_rx_ucast;/**< rx unicast frames */
        a_uint32_t  ns_rx_mcast;/**< rx multi/broadcast frames */
        a_uint64_t  ns_rx_bytes;/**< rx data count (bytes) */
        a_uint64_t  ns_rx_beacons;/**< rx beacon frames */
        a_uint32_t  ns_rx_proberesp;/**< rx probe response frames */
        
        a_uint32_t  ns_rx_dup;/**< rx discard 'cuz dup */
        a_uint32_t  ns_rx_noprivacy;/**< rx w/ wep but privacy off */
        a_uint32_t  ns_rx_wepfail;/**< rx wep processing failed */
        a_uint32_t  ns_rx_demicfail;/**< rx demic failed */
        a_uint32_t  ns_rx_decap;/**< rx decapsulation failed */
        a_uint32_t  ns_rx_defrag;/**< rx defragmentation failed */
        a_uint32_t  ns_rx_disassoc;/**< rx disassociation */
        a_uint32_t  ns_rx_deauth;/**< rx deauthentication */
        a_uint32_t  ns_rx_action;/**< rx action */
        a_uint32_t  ns_rx_decryptcrc;/**< rx decrypt failed on crc */
        a_uint32_t  ns_rx_unauth;/**< rx on unauthorized port */
        a_uint32_t  ns_rx_unencrypted;/**< rx unecrypted w/ privacy */
    
        a_uint32_t  ns_tx_data;/**< tx data frames */
        a_uint32_t  ns_tx_mgmt;/**< tx management frames */
        a_uint32_t  ns_tx_ucast;/**< tx unicast frames */
        a_uint32_t  ns_tx_mcast;/**< tx multi/broadcast frames */
        a_uint64_t  ns_tx_bytes;/**< tx data count (bytes) */
        a_uint32_t  ns_tx_probereq;/**< tx probe request frames */
        a_uint32_t  ns_tx_uapsd;/**< tx on uapsd queue */
        
        a_uint32_t  ns_tx_novlantag;/**< tx discard 'cuz no tag */
        a_uint32_t  ns_tx_vlanmismatch;/**< tx discard 'cuz bad tag */
    
        a_uint32_t  ns_tx_eosplost;/**< uapsd EOSP retried out */
    
        a_uint32_t  ns_ps_discard;/**< ps discard 'cuz of age */
    
        a_uint32_t  ns_uapsd_triggers;/**< uapsd triggers */
    
        /* MIB-related state */
        a_uint32_t  ns_tx_assoc;/**< [re]associations */
        a_uint32_t  ns_tx_assoc_fail;/**< [re]association failures */
        a_uint32_t  ns_tx_auth;/**< [re]authentications */
        a_uint32_t  ns_tx_auth_fail;/**< [re]authentication failures*/
        a_uint32_t  ns_tx_deauth;/**< deauthentications */
        a_uint32_t  ns_tx_deauth_code;/**< last deauth reason */
        a_uint32_t  ns_tx_disassoc;/**< disassociations */
        a_uint32_t  ns_tx_disassoc_code;/**< last disassociation reason */
        a_uint32_t  ns_psq_drops;/**< power save queue drops */
    }data;
} athcfg_wcmd_stastats_t;

/**
 * @brief 11n tx/rx stats
 */
typedef struct athcfg_wcmd_11n_stats {
    a_uint32_t   tx_pkts;/**< total tx data packets */
    a_uint32_t   tx_checks;/**< tx drops in wrong state */
    a_uint32_t   tx_drops;/**< tx drops due to qdepth limit */
    a_uint32_t   tx_minqdepth;/**< tx when h/w queue depth is low */
    a_uint32_t   tx_queue;/**< tx pkts when h/w queue is busy */
    a_uint32_t   tx_comps;/**< tx completions */
    a_uint32_t   tx_stopfiltered;/**< tx pkts filtered for requeueing */
    a_uint32_t   tx_qnull;/**< txq empty occurences */
    a_uint32_t   tx_noskbs;/**< tx no skbs for encapsulations */
    a_uint32_t   tx_nobufs;/**< tx no descriptors */
    a_uint32_t   tx_badsetups;/**< tx key setup failures */
    a_uint32_t   tx_normnobufs;/**< tx no desc for legacy packets */
    a_uint32_t   tx_schednone;/**< tx schedule pkt queue empty */
    a_uint32_t   tx_bars;/**< tx bars sent */
    a_uint32_t   txbar_xretry;/**< tx bars excessively retried */
    a_uint32_t   txbar_compretries;/**< tx bars retried */
    a_uint32_t   txbar_errlast;/**< tx bars last frame failed */
    a_uint32_t   tx_compunaggr;/**< tx unaggregated frame completions */
    a_uint32_t   txunaggr_xretry;/**< tx unaggregated excessive retries */
    a_uint32_t   tx_compaggr;/**< tx aggregated completions */
    a_uint32_t   tx_bawadv;/**< tx block ack window advanced */
    a_uint32_t   tx_bawretries;/**< tx block ack window retries */
    a_uint32_t   tx_bawnorm;/**< tx block ack window additions */
    a_uint32_t   tx_bawupdates;/**< tx block ack window updates */
    a_uint32_t   tx_bawupdtadv;/**< tx block ack window advances */
    a_uint32_t   tx_retries;/**< tx retries of sub frames */
    a_uint32_t   tx_xretries;/**< tx excessive retries of aggregates */
    a_uint32_t   txaggr_noskbs;/**< tx no skbs for aggr encapsualtion */
    a_uint32_t   txaggr_nobufs;/**< tx no desc for aggr */
    a_uint32_t   txaggr_badkeys;/**< tx enc key setup failures */
    a_uint32_t   txaggr_schedwindow;/**< tx no frame scheduled: baw limited */
    a_uint32_t   txaggr_single;/**< tx frames not aggregated */
    a_uint32_t   txaggr_compgood;/**< tx aggr good completions */
    a_uint32_t   txaggr_compxretry;/**< tx aggr excessive retries */
    a_uint32_t   txaggr_compretries;/**< tx aggr unacked subframes */
    a_uint32_t   txunaggr_compretries;/**< tx non-aggr unacked subframes */
    a_uint32_t   txaggr_prepends;/**< tx aggr old frames requeued */
    a_uint32_t   txaggr_filtered;/**< filtered aggr packet */
    a_uint32_t   txaggr_fifo;/**< fifo underrun of aggregate */
    a_uint32_t   txaggr_xtxop;/**< txop exceeded for an aggregate */
    a_uint32_t   txaggr_desc_cfgerr;/**< aggregate descriptor config error */
    a_uint32_t   txaggr_data_urun;/**< data underrun for an aggregate */
    a_uint32_t   txaggr_delim_urun;/**< delimiter underrun for an aggregate */
    a_uint32_t   txaggr_errlast;/**< tx aggr: last sub-frame failed */
    a_uint32_t   txunaggr_errlast;/**< tx non-aggr: last frame failed */
    a_uint32_t   txaggr_longretries;/**< tx aggr h/w long retries */
    a_uint32_t   txaggr_shortretries;/**< tx aggr h/w short retries */
    a_uint32_t   txaggr_timer_exp;/**< tx aggr : tx timer expired */
    a_uint32_t   txaggr_babug;/**< tx aggr : BA bug */
    a_uint32_t   rx_pkts;/**< rx pkts */
    a_uint32_t   rx_aggr;/**< rx aggregated packets */
    a_uint32_t   rx_aggrbadver;/**< rx pkts with bad version */
    a_uint32_t   rx_bars;/**< rx bars */
    a_uint32_t   rx_nonqos;/**< rx non qos-data frames */
    a_uint32_t   rx_seqreset;/**< rx sequence resets */
    a_uint32_t   rx_oldseq;/**< rx old packets */
    a_uint32_t   rx_bareset;/**< rx block ack window reset */
    a_uint32_t   rx_baresetpkts;/**< rx pts indicated due to baw resets */
    a_uint32_t   rx_dup;/**< rx duplicate pkts */
    a_uint32_t   rx_baadvance;/**< rx block ack window advanced */
    a_uint32_t   rx_recvcomp;/**< rx pkt completions */
    a_uint32_t   rx_bardiscard;/**< rx bar discarded */
    a_uint32_t   rx_barcomps;/**< rx pkts unblocked on bar reception */
    a_uint32_t   rx_barrecvs;/**< rx pkt completions on bar reception */
    a_uint32_t   rx_skipped;/**< rx pkt sequences skipped on timeout */
    a_uint32_t   rx_comp_to;/**< rx indications due to timeout */
    a_uint32_t   wd_tx_active;/**< watchdog: tx is active */
    a_uint32_t   wd_tx_inactive;/**< watchdog: tx is not active */
    a_uint32_t   wd_tx_hung;/**< watchdog: tx is hung */
    a_uint32_t   wd_spurious;/**< watchdog: spurious tx hang */
    a_uint32_t   tx_requeue;/**< filter & requeue on 20/40 transitions */
    a_uint32_t   tx_drain_txq;/**< draining tx queue on error */
    a_uint32_t   tx_drain_tid;/**< draining tid buf queue on error */
    a_uint32_t   tx_drain_bufs;/**< buffers drained from pending tid queue */
    a_uint32_t   tx_tidpaused;/**< pausing tx on tid */
    a_uint32_t   tx_tidresumed;/**< resuming tx on tid */
    a_uint32_t   tx_unaggr_filtered;/**< unaggregated tx pkts filtered */
    a_uint32_t   tx_aggr_filtered;/**< aggregated tx pkts filtered */
    a_uint32_t   tx_filtered;/**< total sub-frames filtered */
    a_uint32_t   rx_rb_on;/**< total rb on-s */
    a_uint32_t   rx_rb_off;/**< total rb off-s */
} athcfg_wcmd_11n_stats_t;

/**
 * @brief ampdu info 
 */ 
typedef struct athcfg_wcmd_ampdu_trc {
    a_uint32_t   tr_head;
    a_uint32_t   tr_tail;
    struct trc_entry{
        a_uint16_t   tre_seqst;/**< starting sequence of aggr */
        a_uint16_t   tre_baseqst;/**< starting sequence of ba */
        a_uint32_t   tre_npkts;/**< packets in aggregate */
        a_uint32_t   tre_aggrlen;/**< aggregation length */
        a_uint32_t   tre_bamap0;/**< block ack bitmap word 0 */
        a_uint32_t   tre_bamap1;/**< block ack bitmap word 1 */
    }tr_ents[ATHCFG_WCMD_NUM_TR_ENTS];
} athcfg_wcmd_ampdu_trc_t;

/**
 * @brief phy stats info 
 */ 
typedef struct athcfg_wcmd_phystats{
    a_uint32_t   ast_watchdog;/**< device reset by watchdog */
    a_uint32_t   ast_hardware;/**< fatal hardware error interrupts */
    a_uint32_t   ast_bmiss;/**< beacon miss interrupts */
    a_uint32_t   ast_rxorn;/**< rx overrun interrupts */
    a_uint32_t   ast_rxeol;/**< rx eol interrupts */
    a_uint32_t   ast_txurn;/**< tx underrun interrupts */
    a_uint32_t   ast_txto;/**< tx timeout interrupts */
    a_uint32_t   ast_cst;/**< carrier sense timeout interrupts */
    a_uint32_t   ast_mib;/**< mib interrupts */
    a_uint32_t   ast_tx_packets;/**< packet sent on the interface */
    a_uint32_t   ast_tx_mgmt;/**< management frames transmitted */
    a_uint32_t   ast_tx_discard;/**< frames discarded prior to assoc */
    a_uint32_t   ast_tx_invalid;/**< frames discarded 'cuz device gone */
    a_uint32_t   ast_tx_qstop;/**< tx queue stopped 'cuz full */
    a_uint32_t   ast_tx_encap;/**< tx encapsulation failed */
    a_uint32_t   ast_tx_nonode;/**< no node*/
    a_uint32_t   ast_tx_nobuf;/**< no buf */
    a_uint32_t   ast_tx_nobufmgt;/**< no buffer (mgmt)*/
    a_uint32_t   ast_tx_xretries;/**< too many retries */
    a_uint32_t   ast_tx_fifoerr;/**< FIFO underrun */
    a_uint32_t   ast_tx_filtered;/**< xmit filtered */
    a_uint32_t   ast_tx_timer_exp;/**< tx timer expired */
    a_uint32_t   ast_tx_shortretry;/**< on-chip retries (short) */
    a_uint32_t   ast_tx_longretry;/**< tx on-chip retries (long) */
    a_uint32_t   ast_tx_badrate;/**< tx failed 'cuz bogus xmit rate */
    a_uint32_t   ast_tx_noack;/**< tx frames with no ack marked */
    a_uint32_t   ast_tx_rts;/**< tx frames with rts enabled */
    a_uint32_t   ast_tx_cts;/**< tx frames with cts enabled */
    a_uint32_t   ast_tx_shortpre;/**< tx frames with short preamble */
    a_uint32_t   ast_tx_altrate;/**< tx frames with alternate rate */
    a_uint32_t   ast_tx_protect;/**< tx frames with protection */
    a_uint32_t   ast_rx_orn;/**< rx failed 'cuz of desc overrun */
    a_uint32_t   ast_rx_crcerr;/**< rx failed 'cuz of bad CRC */
    a_uint32_t   ast_rx_fifoerr;/**< rx failed 'cuz of FIFO overrun */
    a_uint32_t   ast_rx_badcrypt;/**< rx failed 'cuz decryption */
    a_uint32_t   ast_rx_badmic;/**< rx failed 'cuz MIC failure */
    a_uint32_t   ast_rx_phyerr;/**< rx PHY error summary count */
    a_uint32_t   ast_rx_phy[64];/**< rx PHY error per-code counts */
    a_uint32_t   ast_rx_tooshort;/**< rx discarded 'cuz frame too short */
    a_uint32_t   ast_rx_toobig;/**< rx discarded 'cuz frame too large */
    a_uint32_t   ast_rx_nobuf;/**< rx setup failed 'cuz no skbuff */
    a_uint32_t   ast_rx_packets;/**< packet recv on the interface */
    a_uint32_t   ast_rx_mgt;/**< management frames received */
    a_uint32_t   ast_rx_ctl;/**< control frames received */
    a_int8_t     ast_tx_rssi_combined;/**< tx rssi of last ack [combined] */
    a_int8_t     ast_tx_rssi_ctl0;/**< tx rssi of last ack [ctl, chain 0] */
    a_int8_t     ast_tx_rssi_ctl1;/**< tx rssi of last ack [ctl, chain 1] */
    a_int8_t     ast_tx_rssi_ctl2;/**< tx rssi of last ack [ctl, chain 2] */
    a_int8_t     ast_tx_rssi_ext0;/**< tx rssi of last ack [ext, chain 0] */
    a_int8_t     ast_tx_rssi_ext1;/**< tx rssi of last ack [ext, chain 1] */
    a_int8_t     ast_tx_rssi_ext2;/**< tx rssi of last ack [ext, chain 2] */
    a_int8_t     ast_rx_rssi_combined;/**< rx rssi from histogram [combined]*/
    a_int8_t     ast_rx_rssi_ctl0;/**< rx rssi from histogram [ctl, chain 0] */
    a_int8_t     ast_rx_rssi_ctl1;/**< rx rssi from histogram [ctl, chain 1] */
    a_int8_t     ast_rx_rssi_ctl2;/**< rx rssi from histogram [ctl, chain 2] */
    a_int8_t     ast_rx_rssi_ext0;/**< rx rssi from histogram [ext, chain 0] */
    a_int8_t     ast_rx_rssi_ext1;/**< rx rssi from histogram [ext, chain 1] */
    a_int8_t     ast_rx_rssi_ext2;/**< rx rssi from histogram [ext, chain 2] */
    a_uint32_t   ast_be_xmit;/**< beacons transmitted */
    a_uint32_t   ast_be_nobuf;/**< no skbuff available for beacon */
    a_uint32_t   ast_per_cal;/**< periodic calibration calls */
    a_uint32_t   ast_per_calfail;/**< periodic calibration failed */
    a_uint32_t   ast_per_rfgain;/**< periodic calibration rfgain reset */
    a_uint32_t   ast_rate_calls;/**< rate control checks */
    a_uint32_t   ast_rate_raise;/**< rate control raised xmit rate */
    a_uint32_t   ast_rate_drop;/**< rate control dropped xmit rate */
    a_uint32_t   ast_ant_defswitch;/**< rx/default antenna switches */
    a_uint32_t   ast_ant_txswitch;/**< tx antenna switches */
    a_uint32_t   ast_ant_rx[8];/**< rx frames with antenna */
    a_uint32_t   ast_ant_tx[8];/**< tx frames with antenna */
    a_uint32_t   ast_suspend;/**< driver suspend calls */
    a_uint32_t   ast_resume;/**< driver resume calls  */
    a_uint32_t   ast_shutdown;/**< driver shutdown calls  */
    a_uint32_t   ast_init;/**< driver init calls  */
    a_uint32_t   ast_stop;/**< driver stop calls  */
    a_uint32_t   ast_reset;/**< driver resets      */
    a_uint32_t   ast_nodealloc;/**< nodes allocated    */
    a_uint32_t   ast_nodefree;/**< nodes deleted      */
    a_uint32_t   ast_keyalloc;/**< keys allocated     */
    a_uint32_t   ast_keydelete;/**< keys deleted       */
    a_uint32_t   ast_bstuck;/**< beacon stuck       */
    a_uint32_t   ast_draintxq;/**< drain tx queue     */
    a_uint32_t   ast_stopdma;/**< stop tx queue dma  */
    a_uint32_t   ast_stoprecv;/**< stop recv          */
    a_uint32_t   ast_startrecv;/**< start recv         */
    a_uint32_t   ast_flushrecv;/**< flush recv         */
    a_uint32_t   ast_chanchange;/**< channel changes    */
    a_uint32_t   ast_fastcc;/**< Number of fast channel changes */
    a_uint32_t   ast_fastcc_errs;/**< Number of failed fast channel changes */
    a_uint32_t   ast_chanset;/**< channel sets       */
    a_uint32_t   ast_cwm_mac;/**< CWM - mac mode switch */
    a_uint32_t   ast_cwm_phy;/**< CWM - phy mode switch */
    a_uint32_t   ast_cwm_requeue;/**< CWM - requeue dest node packets */
    a_uint32_t   ast_rx_delim_pre_crcerr;/**< pre-delimit crc errors */
    a_uint32_t   ast_rx_delim_post_crcerr;/**< post-delimit crc errors */
    a_uint32_t   ast_rx_decrypt_busyerr;/**< decrypt busy errors */

    athcfg_wcmd_11n_stats_t  ast_11n;/**< 11n statistics */
    athcfg_wcmd_ampdu_trc_t  ast_trc;/**< ampdu trc */
} athcfg_wcmd_phystats_t;

typedef struct athcfg_wcmd_hst_11n_phystats{
    a_uint32_t   tx_pkts;            /* total tx data packets */
    a_uint32_t   tx_checks;          /* tx drops in wrong state */
    a_uint32_t   tx_drops;           /* tx drops due to qdepth limit */
    a_uint32_t   tx_queue;           /* tx pkts when h/w queue is busy */
    a_uint32_t   tx_normnobufs;      /* tx no desc for legacy packets */
    a_uint32_t   tx_schednone;       /* tx schedule pkt queue empty */
    a_uint32_t   txaggr_noskbs;      /* tx no skbs for aggr encapsualtion */
    a_uint32_t   txaggr_nobufs;      /* tx no desc for aggr */
    a_uint32_t   txaggr_badkeys;     /* tx enc key setup failures */
    a_uint32_t   rx_pkts;            /* rx pkts */
    a_uint32_t   rx_aggr;            /* rx aggregated packets */
    a_uint32_t   rx_aggrbadver;      /* rx pkts with bad version */
    a_uint32_t   rx_bars;            /* rx bars */
    a_uint32_t   rx_nonqos;          /* rx non qos-data frames */
    a_uint32_t   rx_seqreset;        /* rx sequence resets */
    a_uint32_t   rx_oldseq;          /* rx old packets */
    a_uint32_t   rx_bareset;         /* rx block ack window reset */
    a_uint32_t   rx_baresetpkts;     /* rx pts indicated due to baw resets */
    a_uint32_t   rx_dup;             /* rx duplicate pkts */
    a_uint32_t   rx_baadvance;       /* rx block ack window advanced */
    a_uint32_t   rx_recvcomp;        /* rx pkt completions */
    a_uint32_t   rx_bardiscard;      /* rx bar discarded */
    a_uint32_t   rx_barcomps;        /* rx pkts unblocked on bar reception */
    a_uint32_t   rx_barrecvs;        /* rx pkt completions on bar reception */
    a_uint32_t   rx_skipped;         /* rx pkt sequences skipped on timeout */
    a_uint32_t   rx_comp_to;         /* rx indications due to timeout */
    a_uint32_t   wd_tx_active;       /* watchdog: tx is active */
    a_uint32_t   wd_tx_inactive;     /* watchdog: tx is not active */
    a_uint32_t   wd_tx_hung;         /* watchdog: tx is hung */
    a_uint32_t   wd_spurious;        /* watchdog: spurious tx hang */
    a_uint32_t   tx_requeue;         /* filter & requeue on 20/40 transitions */
    a_uint32_t   tx_drain_tid;       /* draining tid buf queue on error */
    a_uint32_t   tx_drain_bufs;      /* buffers drained from pending tid queue */
    a_uint32_t   tx_tidpaused;       /* pausing tx on tid */
    a_uint32_t   tx_tidresumed;      /* resuming tx on tid */
    a_uint32_t   tx_unaggr_filtered; /* unaggregated tx pkts filtered */
    a_uint32_t   tx_filtered;        /* total sub-frames filtered */
    a_uint32_t   rx_rb_on;           /* total rb on-s */
    a_uint32_t   rx_rb_off;          /* total rb off-s */ 

}athcfg_wcmd_hst_11n_phystats_t;

typedef struct athcfg_wcmd_hst_phystats{
    a_uint32_t   ast_watchdog;   /* device reset by watchdog */
    a_uint64_t   ast_tx_packets; /* packet sent on the interface */
    a_uint32_t   ast_tx_discard; /* frames discarded prior to assoc */
    a_uint32_t   ast_tx_invalid; /* frames discarded 'cuz device gone */
    a_uint32_t   ast_tx_qstop;   /* tx queue stopped 'cuz full */
    a_uint32_t   ast_tx_nonode;  /* tx failed 'cuz no node */    
    a_uint32_t   ast_tx_badrate; /* tx failed 'cuz bogus xmit rate */
    a_uint32_t   ast_tx_noack;   /* tx frames with no ack marked */    
    a_uint32_t   ast_tx_cts; /* tx frames with cts enabled */
    a_uint32_t   ast_tx_shortpre;/* tx frames with short preamble */
    a_uint32_t   ast_rx_crcerr;  /* rx failed 'cuz of bad CRC */
    a_uint32_t   ast_rx_fifoerr; /* rx failed 'cuz of FIFO overrun */
    a_uint32_t   ast_rx_badcrypt;/* rx failed 'cuz decryption */
    a_uint32_t   ast_rx_badmic;  /* rx failed 'cuz MIC failure */
    a_uint32_t   ast_rx_phyerr;  /* rx PHY error summary count */
    a_uint32_t   ast_rx_phy[64]; /* rx PHY error per-code counts */
    a_uint32_t   ast_rx_tooshort;/* rx discarded 'cuz frame too short */
    a_uint32_t   ast_rx_toobig;  /* rx discarded 'cuz frame too large */    
    a_uint64_t   ast_rx_packets; /* packet recv on the interface */
    a_uint32_t   ast_rx_mgt; /* management frames received */
    a_uint32_t   ast_rx_ctl; /* control frames received */
    a_int8_t     ast_rx_rssi_combined;/* rx rssi from histogram [combined]*/
    a_int8_t     ast_rx_rssi_ctl0; /* rx rssi from histogram [ctl, chain 0] */
    a_int8_t     ast_rx_rssi_ctl1; /* rx rssi from histogram [ctl, chain 1] */
    a_int8_t     ast_rx_rssi_ctl2; /* rx rssi from histogram [ctl, chain 2] */
    a_int8_t     ast_rx_rssi_ext0; /* rx rssi from histogram [ext, chain 0] */
    a_int8_t     ast_rx_rssi_ext1; /* rx rssi from histogram [ext, chain 1] */
    a_int8_t     ast_rx_rssi_ext2; /* rx rssi from histogram [ext, chain 2] */
    a_int8_t     ast_bc_rx_rssi_combined;/* beacon rx rssi from histogram [combined]*/
    a_int8_t     ast_bc_rx_rssi_ctl0; /* beacon rx rssi from histogram [ctl, chain 0] */
    a_int8_t     ast_bc_rx_rssi_ctl1; /* beacon rx rssi from histogram [ctl, chain 1] */
    a_int8_t     ast_bc_rx_rssi_ctl2; /* beacon rx rssi from histogram [ctl, chain 2] */
    a_uint32_t   ast_be_nobuf;   /* no skbuff available for beacon */
    a_uint32_t   ast_per_cal;    /* periodic calibration calls */
    a_uint32_t   ast_per_calfail;/* periodic calibration failed */
    a_uint32_t   ast_per_rfgain; /* periodic calibration rfgain reset */
    a_uint32_t   ast_rate_calls; /* rate control checks */
    a_uint32_t   ast_rate_raise; /* rate control raised xmit rate */
    a_uint32_t   ast_rate_drop;  /* rate control dropped xmit rate */
    a_uint32_t   ast_ant_txswitch;/* tx antenna switches */
    a_uint32_t   ast_ant_rx[8];  /* rx frames with antenna */ 
    a_uint32_t   ast_suspend;    /* driver suspend calls */
    a_uint32_t   ast_resume;     /* driver resume calls  */
    a_uint32_t   ast_shutdown;   /* driver shutdown calls  */
    a_uint32_t   ast_init;       /* driver init calls  */
    a_uint32_t   ast_stop;       /* driver stop calls  */
    a_uint32_t   ast_reset;      /* driver resets      */
    a_uint32_t   ast_nodealloc;  /* nodes allocated    */
    a_uint32_t   ast_nodefree;   /* nodes deleted      */
    a_uint32_t   ast_keyalloc;   /* keys allocateds    */
    a_uint32_t   ast_keydelete;   /* keys deleted    */
    a_uint32_t   ast_bstuck;     /* beacon stuck       */
    a_uint32_t   ast_startrecv;  /* start recv         */
    a_uint32_t   ast_flushrecv;  /* flush recv         */
    a_uint32_t   ast_chanchange; /* channel changes    */
    a_uint32_t   ast_fastcc;     /* Number of fast channel changes */
    a_uint32_t   ast_fastcc_errs;/* Number of failed fast channel changes */
    a_uint32_t   ast_chanset;    /* channel sets       */
    a_uint32_t   ast_cwm_mac;    /* CWM - mac mode switch */
    a_uint32_t   ast_cwm_phy;    /* CWM - phy mode switch */
    a_uint32_t   ast_cwm_requeue;/* CWM - requeue dest node packets */
    a_uint32_t   ast_rx_delim_pre_crcerr; /* pre-delimiter crc errors */
    a_uint32_t   ast_rx_delim_post_crcerr; /* post-delimiter crc errors */
    a_uint32_t   ast_rx_decrypt_busyerr; /* decrypt busy errors */

    a_uint32_t   ast_rx_rate; 			/* current rx rate */
    /* 11n statistics */
    athcfg_wcmd_hst_11n_phystats_t ast_11n; 
    
}athcfg_wcmd_hst_phystats_t;

typedef struct athcfg_wcmd_tgt_11n_phystats{
    a_uint32_t   tx_tgt;         /* tx data pkts recieved on target */  
    a_uint32_t   tx_nframes;         /* no. of frames aggregated */
    a_uint32_t   tx_comps;           /* tx completions */
    a_uint32_t   tx_qnull;           /* txq empty occurences */
    a_uint32_t   tx_compunaggr;      /* tx unaggregated frame completions */ 
    a_uint32_t   tx_compaggr;        /* tx aggregated completions */
    a_uint32_t   tx_rate;            /* tx rate in Kbps*/
    a_uint32_t   rx_rate;            /* rx rate in Kbps */
    a_int8_t     ast_tx_rssi_combined;/* rx rssi from histogram [combined]*/
    a_int8_t     ast_tx_rssi_ctl0; /* rx rssi from histogram [ctl, chain 0] */
    a_int8_t     ast_tx_rssi_ctl1; /* rx rssi from histogram [ctl, chain 1] */
    a_int8_t     ast_tx_rssi_ctl2; /* rx rssi from histogram [ctl, chain 2] */
    a_int8_t     ast_tx_rssi_ext0; /* rx rssi from histogram [ext, chain 0] */
    a_int8_t     ast_tx_rssi_ext1; /* rx rssi from histogram [ext, chain 1] */
    a_int8_t     ast_tx_rssi_ext2; /* rx rssi from histogram [ext, chain 2] */
    a_uint32_t   txaggr_compgood;    /* tx aggr good completions */
    a_uint32_t   txaggr_compretries; /* tx aggr unacked subframes */
    a_uint32_t   txaggr_prepends;    /* tx aggr old frames requeued */
    a_uint32_t   txaggr_data_urun;   /* data underrun for an aggregate */
    a_uint32_t   txaggr_delim_urun;  /* delimiter underrun for an aggr */
    a_uint32_t   txaggr_errlast;     /* tx aggr: last sub-frame failed */
    a_uint32_t   txaggr_longretries; /* tx aggr h/w long retries */
    a_uint32_t   tx_drain_txq;       /* draining tx queue on error */
    a_uint32_t   tx_minqdepth;       /* tx when h/w queue depth is low */
    a_uint32_t   tx_stopfiltered;    /* tx pkts filtered for requeueing */
    a_uint32_t   tx_noskbs;          /* tx no skbs for encapsulations */
    a_uint32_t   tx_nobufs;          /* tx no descriptors */
    a_uint32_t   tx_badsetups;       /* tx key setup failures */
    a_uint32_t   tx_bars;            /* tx bars sent */
    a_uint32_t   txbar_xretry;       /* tx bars excessively retried */
    a_uint32_t   txbar_compretries;  /* tx bars retried */
    a_uint32_t   txbar_errlast;      /* tx bars last frame failed */
    a_uint32_t   txunaggr_xretry;    /* tx unaggregated excessive retries */
    a_uint32_t   tx_bawadv;          /* tx block ack window advanced */
    a_uint32_t   tx_bawretries;      /* tx block ack window retries */
    a_uint32_t   tx_bawnorm;         /* tx block ack window additions */
    a_uint32_t   tx_bawupdates;      /* tx block ack window updates */
    a_uint32_t   tx_bawupdtadv;      /* tx block ack window advances */
    a_uint32_t   tx_xretries;        /* tx excessive retries of aggr */
    a_uint32_t   txaggr_schedwindow; /* tx no frame scheduled: baw lim */
    a_uint32_t   txaggr_compxretry;  /* tx aggr excessive retries */
    a_uint32_t   txunaggr_compretries; /* tx non-aggr unacked subframes */
    a_uint32_t   txaggr_filtered;    /* filtered aggr packet */
    a_uint32_t   txaggr_fifo;        /* fifo underrun of aggregate */
    a_uint32_t   txaggr_xtxop;       /* txop exceeded for an aggregate */
    a_uint32_t   txaggr_desc_cfgerr; /* aggr descriptor config error */
    a_uint32_t   txunaggr_errlast;   /* tx non-aggr: last frame failed */
    a_uint32_t   txaggr_shortretries;/* tx aggr h/w short retries */
    a_uint32_t   txaggr_timer_exp;   /* tx aggr : tx timer expired */
    a_uint32_t   txaggr_babug;       /* tx aggr : BA bug */
    a_uint32_t   tx_aggr_filtered;   /* aggregated tx pkts filtered */
    a_uint32_t   tx_tgt_drain_bufs;  /* draining target buffers */

}athcfg_wcmd_tgt_11n_phystats_t;


typedef struct athcfg_wcmd_tgt_phystats{
     a_uint32_t   ast_hardware;   /* fatal hardware error interrupts */
    a_uint32_t   ast_bmiss;  /* beacon miss interrupts */
    a_uint32_t   ast_rxorn;  /* rx overrun interrupts */
    a_uint32_t   ast_rxeol;  /* rx eol interrupts */
    a_uint32_t   ast_txurn;  /* tx underrun interrupts */
    a_uint32_t   ast_txto;   /* tx timeout interrupts */
    a_uint32_t   ast_cst;    /* carrier sense timeout interrupts */
    a_uint32_t   ast_mib;    /* mib interrupts */
    a_uint32_t   ast_tx_mgmt;    /* management frames transmitted */
    a_uint32_t   ast_tx_encap;   /* tx encapsulation failed */
    a_uint32_t   ast_tx_nobuf;   /* tx failed 'cuz no tx buffer (data) */
    a_uint32_t   ast_tx_nobufmgt;/* tx failed 'cuz no tx buffer (mgmt)*/
    a_uint32_t   ast_tx_xretries;/* tx failed 'cuz too many retries */
    a_uint32_t   ast_tx_fifoerr; /* tx failed 'cuz FIFO underrun */
    a_uint32_t   ast_tx_filtered;/* tx failed 'cuz xmit filtered */
    a_uint32_t   ast_tx_timer_exp;/* tx timer expired */
    a_uint32_t   ast_tx_shortretry;/* tx on-chip retries (short) */
    a_uint32_t   ast_tx_longretry;/* tx on-chip retries (long) */
    a_uint32_t   ast_tx_rts; /* tx frames with rts enabled */
    a_uint32_t   ast_tx_altrate; /* tx frames with alternate rate */
    a_uint32_t   ast_tx_protect; /* tx frames with protection */
    a_uint32_t   ast_rx_orn; /* rx failed 'cuz of desc overrun */
    a_uint32_t   ast_rx_nobuf;   /* rx setup failed 'cuz no skbuff */
    int8_t      ast_tx_rssi_combined;/* tx rssi of last ack [combined] */
    int8_t      ast_tx_rssi_ctl0;    /* tx rssi of last ack [ctl,chain0] */
    int8_t      ast_tx_rssi_ctl1;    /* tx rssi of last ack [ctl,chain1] */
    int8_t      ast_tx_rssi_ctl2;    /* tx rssi of last ack [ctl,chain2] */
    int8_t      ast_tx_rssi_ext0;    /* tx rssi of last ack [ext,chain0] */
    int8_t      ast_tx_rssi_ext1;    /* tx rssi of last ack [ext,chain1] */
    int8_t      ast_tx_rssi_ext2;    /* tx rssi of last ack [ext,chain2] */
    a_uint32_t   ast_be_xmit;        /* beacon xmit */
    a_uint32_t   ast_ant_defswitch;/* rx/default antenna switches */
    a_uint32_t   ast_ant_tx[8];  /* tx frames with antenna */
    a_uint32_t   ast_draintxq;   /* drain tx queue     */
    a_uint32_t   ast_stopdma;    /* stop tx queue dma  */
    a_uint32_t   ast_stoprecv;   /* stop recv          */
   
    /* 11n statistics */
    athcfg_wcmd_tgt_11n_phystats_t ast_11n_tgt; 

}athcfg_wcmd_tgt_phystats_t;
/**
 * @brief diag info 
 */ 
typedef struct athcfg_wcmd_diag{
    a_int8_t     ad_name[ATHCFG_WCMD_NAME_SIZE];/**< if name*/
    a_uint16_t   ad_id;
    a_uint16_t   ad_in_size;/**< pack to fit, yech */
    a_uint8_t    *ad_in_data;
    a_uint8_t    *ad_out_data;
    a_uint32_t   ad_out_size;
}athcfg_wcmd_diag_t;

/*
 * Device phyerr ioctl info
 */
typedef struct athcfg_wcmd_phyerr{
    a_int8_t     ad_name[ATHCFG_WCMD_NAME_SIZE];/**< if name, e.g. "ath0" */
    a_uint16_t   ad_id;
    a_uint16_t   ad_in_size;                /**< pack to fit, yech */
    a_uint8_t    *ad_in_data;
    a_uint8_t    *ad_out_data;
    a_uint32_t   ad_out_size;
}athcfg_wcmd_phyerr_t;

/**
 * @brief cwm-info
 */
typedef struct athcfg_wcmd_cwminfo {
    a_uint32_t  ci_chwidth; /**< channel width */
    a_uint32_t  ci_macmode; /**< MAC mode */
    a_uint32_t  ci_phymode; /**< Phy mode */
    a_uint32_t  ci_extbusyper; /**< extension busy (percent) */
} athcfg_wcmd_cwminfo_t;

/**
 * @brief cwm-dbg
 */
typedef struct athcfg_wcmd_cwmdbg {
    athcfg_wcmd_cwm_cmd_t    dc_cmd;/**< dbg commands*/
    athcfg_wcmd_cwm_event_t  dc_arg;/**< events*/
} athcfg_wcmd_cwmdbg_t;    

/**
 * @brief device cwm info
 */
typedef struct athcfg_wcmd_cwm{
    a_uint32_t      type;
    union{
        athcfg_wcmd_cwmdbg_t   dbg;
        athcfg_wcmd_cwminfo_t  info;
    }cwm;
}athcfg_wcmd_cwm_t;

/**
 * @brief Helpers to access the CWM structures
 */
#define cwm_dbg         cwm.dbg
#define cwm_info        cwm.info

/**
 * @brief eth tool info
 */
typedef struct athcfg_wcmd_ethtool{
    a_uint32_t  cmd;
    a_int8_t    driver[ATHCFG_WCMD_DRIVSIZ];/**< driver short name */
    a_int8_t    version[ATHCFG_WCMD_VERSIZ];/**< driver ver string */
    a_int8_t    fw_version[ATHCFG_WCMD_FIRMSIZ];/**< firmware ver string*/
    a_int8_t    bus_info[ATHCFG_WCMD_BUSINFO_LEN];/**< Bus info */ 
    a_int8_t    reserved1[32];
    a_int8_t    reserved2[16];
    a_uint32_t  n_stats;/**< number of u64's from ETHTOOL_GSTATS */
    a_uint32_t  testinfo_len;   
    a_uint32_t  eedump_len;/**< Size of data from EEPROM(bytes) */
    a_uint32_t  regdump_len;/**< Size of data from REG(bytes) */
}athcfg_wcmd_ethtool_t ;

typedef struct athcfg_wcmd_ethtool_info{
    athcfg_wcmd_ethtool_cmd_t     cmd;
    athcfg_wcmd_ethtool_t          drv;
}athcfg_wcmd_ethtool_info_t;

/** 
 * @brief vap create flag info 
 */ 
typedef enum athcfg_wcmd_vapcreate_flags{
    ATHCFG_WCMD_CLONE_BSSID=0x1,/**< allocate unique mac/bssid */
    ATHCFG_WCMD_NO_STABEACONS/**< Do not setup the sta beacon timers*/ 
}athcfg_wcmd_vapcreate_flags_t;

/**
 * @brief VAP info structure used during VAPCREATE
 */
typedef struct athcfg_wcmd_vapinfo{
    a_uint8_t                       icp_name[ATHCFG_WCMD_NAME_SIZE];
    athcfg_wcmd_opmode_t           icp_opmode;/**< operating mode */
    athcfg_wcmd_vapcreate_flags_t  icp_flags;
}athcfg_wcmd_vapinfo_t;

/**
 * @brief ath stats info
 */
typedef struct athcfg_wcmd_devstats{
    a_uint64_t   rx_packets;/**< total packets received       */
    a_uint64_t   tx_packets;/**< total packets transmitted    */
    a_uint64_t   rx_bytes;/**< total bytes received         */
    a_uint64_t   tx_bytes;/**< total bytes transmitted      */
    a_uint64_t   rx_errors;/**< bad packets received         */
    a_uint64_t   tx_errors;/**< packet transmit problems     */
    a_uint64_t   rx_dropped;/**< no space in linux buffers    */
    a_uint64_t   tx_dropped;/**< no space available in linux  */
    a_uint64_t   multicast;/**< multicast packets received   */
    a_uint64_t   collisions;
    
    /* detailed rx_errors: */
    a_uint64_t   rx_length_errors;
    a_uint64_t   rx_over_errors;/**< receiver ring buff overflow  */
    a_uint64_t   rx_crc_errors;/**< recved pkt with crc error    */
    a_uint64_t   rx_frame_errors;/**< recv'd frame alignment error */
    a_uint64_t   rx_fifo_errors;/**< recv'r fifo overrun          */
    a_uint64_t   rx_missed_errors;/**< receiver missed packet       */
    
    /* detailed tx_errors */
    a_uint64_t   tx_aborted_errors;
    a_uint64_t   tx_carrier_errors;
    a_uint64_t   tx_fifo_errors;
    a_uint64_t   tx_heartbeat_errors;
    a_uint64_t   tx_window_errors;
    
    /* for cslip etc */
    a_uint64_t   rx_compressed;
    a_uint64_t   tx_compressed;
}athcfg_wcmd_devstats_t;

/**
 * @brief mtu set/get/enable/disable info
 */
typedef  a_uint32_t     athcfg_wcmd_mtu_t;
/**
 * @brief turbo
 */
typedef  a_uint32_t     athcfg_wcmd_turbo_t;

/**
 * @brief Used for noise floor parameter setting 
 *  exchange from the driver
 */ 
typedef struct athcfg_wcmd_noisefloor_cmd {
    a_uint16_t get;
    a_int16_t val;
}athcfg_wcmd_noisefloor_cmd_t;

/**
 * @brief Used for antenna diversity setting 
 *  exchange from the driver
 */ 
typedef struct athcfg_wcmd_ant_div_cmd {
    a_uint16_t get_flag;
    a_uint16_t val;
}athcfg_wcmd_ant_div_cmd_t;

/**
 * @brief Used for user specific debugging info 
 *  exchange from the driver
 */ 
typedef struct athcfg_wcmd_dbg_info{
    athcfg_wcmd_dbg_type_t     type;
    a_uint8_t                  data[ATHCFG_WCMD_DBG_MAXLEN];
}athcfg_wcmd_dbg_info_t;

/**
 * @brief Used for user specific debugging info 
 *  exchange from the driver
 */ 
typedef struct athcfg_wcmd_reg{
    a_uint32_t addr;
    a_uint32_t val;
}athcfg_wcmd_reg_t;

/**
 * @brief Used for tx99 function
 */ 
typedef struct athcfg_wcmd_tx99 {
    a_uint32_t freq;	/* tx frequency (MHz) */
    a_uint32_t htmode;	/* tx bandwidth (HT20/HT40) */
	a_uint32_t htext;	/* extension channel offset (0(none), 1(plus) and 2(minus)) */
	a_uint32_t rate;	/* Kbits/s */
	a_uint32_t power;  	/* (dBm) */
	a_uint32_t txmode; 	/* wireless mode, 11NG(8), auto-select(0) */
	a_uint32_t rcvmode; /* receive mode 0: disable continuous receive 1: enable continuous receive */
	a_uint32_t chanmask; /* tx chain mask */
	a_uint32_t type;
} athcfg_wcmd_tx99_t;

/**
 * @brief Used for pktlog event list
 */ 
typedef struct athcfg_wcmd_pktlog_eventlist{
    a_uint32_t eventlist;
}athcfg_wcmd_pktlog_eventlist_t;

typedef struct athcfg_wcmd_pktlog_read {
    a_uint32_t nbytes;
    a_uint8_t data[4096];
} athcfg_wcmd_pktlog_read_t;

#define PKTLOG_EVENT_TX_MASK (0x1)
#define PKTLOG_EVENT_RX_MASK (0x2)
#define PKTLOG_EVENT_RCF_MASK (0x4)
#define PKTLOG_EVENT_RCU_MASK (0x8)
#define PKTLOG_EVENT_TCP_MASK (0x10)

typedef struct adf_net_wcmd_pktlog_read {
        a_uint32_t nbytes; /*Number of bytes read*/
        a_uint8_t data[4096];
} adf_net_wcmd_pktlog_read_t;

/**
 * @brief radio set/get
 */
typedef struct athcfg_wcmd_radio {
    a_uint16_t          enable;
}athcfg_wcmd_radio_t;

/**
 * @brief specify channel 
 */
typedef struct athcfg_wcmd_spec_chan_scan{
    a_uint8_t   spec_chan_scan;     //scan_specific_channel_flag
    a_uint16_t  spec_chan_num;     //scan_specific_channel
}athcfg_wcmd_spec_chan_scan_t;

/**
 * @brief hidden ssid scan set/get
 */
typedef struct athcfg_wcmd_hiddenssid_scan{
    a_uint16_t          enable;
}athcfg_wcmd_hiddenssid_scan_t;

/**
 *  * @brief htrates get
 *   */
typedef struct athcfg_wcmd_htrates{
    a_uint32_t  htrates[ATHCFG_WCMD_RATE_MAXSIZE];
    a_uint32_t  len;
}athcfg_wcmd_htrates_t;

/**
 * @brief ic_next-scandelay set/get info
 */
typedef a_uint32_t     athcfg_wcmd_ic_nextscandelay_t;


/**
 * @brief autoassociation set/get info
 */
typedef a_uint8_t     athcfg_wcmd_ic_autoassoc_t;

/**
 * @brief  get product-info
 */
typedef struct athcfg_wcmd_product_info {
    a_uint16_t idVendor;
    a_uint16_t idProduct;
    a_uint8_t  product[64];
    a_uint8_t  manufacturer[64];
    a_uint8_t  serial[64];
} athcfg_wcmd_product_info_t;

typedef union athcfg_wcmd_data{
    athcfg_wcmd_vapname_t          vapname;
    athcfg_wcmd_bssid_t            bssid;
    athcfg_wcmd_nickname_t         nickname;
    athcfg_wcmd_ssid_t             essid;
    athcfg_wcmd_rts_th_t           rts;
    athcfg_wcmd_frag_th_t          frag;
    athcfg_wcmd_ic_caps_t          ic_caps;
    athcfg_wcmd_freq_t             freq;
    athcfg_wcmd_retries_t          retries;
    athcfg_wcmd_txpower_t          txpower;
    athcfg_wcmd_txpowlimit_t       txpowlimit;
    athcfg_wcmd_vaplist_t          vaplist;
    athcfg_wcmd_phymode_t          phymode;
    athcfg_wcmd_vapmode_t          vapmode;
    athcfg_wcmd_devcap_t           devcap;
    athcfg_wcmd_turbo_t            turbo;
    athcfg_wcmd_param_t            param;
    athcfg_wcmd_optie_t            optie;
    athcfg_wcmd_appie_t            appie;
    athcfg_wcmd_filter_type_t      filter;
    athcfg_wcmd_addba_t            addba;
    athcfg_wcmd_delba_t            delba;
    athcfg_wcmd_addba_status_t     addba_status;
    athcfg_wcmd_addba_resp_t       addba_resp;
    athcfg_wcmd_keyinfo_t          key;
    athcfg_wcmd_mlme_t             mlme;
    athcfg_wcmd_chanlist_t         chanlist;
    athcfg_wcmd_chaninfo_t         chaninfo;
    athcfg_wcmd_wmmparaminfo_t     wmmparam;
    athcfg_wcmd_wpaie_t            wpaie;
    athcfg_wcmd_wscie_t            wscie;
    athcfg_wcmd_power_t            power;
    athcfg_wcmd_diag_t             dev_diag;
    athcfg_wcmd_phyerr_t           phyerr;
    athcfg_wcmd_cwm_t              cwm;
    athcfg_wcmd_ethtool_info_t     ethtool;
    athcfg_wcmd_vapinfo_t          vap_info;/**< during vapcreate*/

    athcfg_wcmd_mtu_t              mtu;
    athcfg_ethaddr_t               mac;/*MAC addr of VAP or Dev */
    athcfg_wcmd_stainfo_t          *station;
    athcfg_wcmd_scan_t             *scan;
    athcfg_wcmd_vapparam_range_t   *range;
    athcfg_wcmd_stastats_t         *stats_sta;
    athcfg_wcmd_vapstats_t         *stats_vap;
    athcfg_wcmd_phystats_t         *stats_phy;
    athcfg_wcmd_devstats_t         *stats_dev;
    athcfg_wcmd_hst_phystats_t     *stats_hst;/* Host statistics */
    athcfg_wcmd_tgt_phystats_t     *stats_tgt;/* Target statistics */
    
    a_uint32_t                      datum;/*for sysctl*/
    athcfg_wcmd_noisefloor_cmd_t    nf; /* Noise floor parameters */
    athcfg_wcmd_ant_div_cmd_t       divParam; /* Antenna diversity parameters */
    athcfg_wcmd_dbg_info_t         dbg_info;/* Debugging purpose*/
    athcfg_wcmd_reg_t              reg;
    athcfg_wcmd_pktlog_eventlist_t pktlog_eventlist; /* Pktlog event list */
    a_uint32_t             size; /* Log buffer size for pktlog */
    adf_net_wcmd_pktlog_read_t     *pktlog_read_info;
    athcfg_wcmd_ic_nextscandelay_t ic_scandelay;          
    athcfg_wcmd_ic_autoassoc_t     ic_autoassoc;
    athcfg_wcmd_product_info_t     product_info;        
    athcfg_wcmd_radio_t             radio;
    athcfg_wcmd_spec_chan_scan_t   ic_specchanscan;
    athcfg_wcmd_hiddenssid_scan_t  ic_hiddenssidscan;
    athcfg_wcmd_testmode_t          testmode;
    athcfg_wcmd_tx99_t				tx99;
    athcfg_wcmd_htrates_t          *htrates;
    a_uint32_t                     wmi_stats_timeout;
} athcfg_wcmd_data_t;

/**
 * @brief ioctl structure to configure the wireless interface.
 */ 
typedef struct athcfg_wcmd{
    char                 if_name[ATHCFG_WCMD_NAME_SIZE];/**< Interface name */
    athcfg_wcmd_type_t   type;             /**< Type of wcmd */
    athcfg_wcmd_data_t   data;             /**< Data */       
} athcfg_wcmd_t;
/**
 * @brief helper macros
 */
#define d_vapname               data.vapname
#define d_bssid                 data.bssid
#define d_nickname              data.nickname
#define d_essid                 data.essid
#define d_rts                   data.rts
#define d_frag                  data.frag
#define d_iccaps                data.ic_caps
#define d_freq                  data.freq
#define d_retries               data.retries
#define d_txpower               data.txpower
#define d_txpowlimit            data.txpowlimit
#define d_vaplist               data.vaplist
#define d_scan                  data.scan
#define d_phymode               data.phymode
#define d_vapmode               data.vapmode
#define d_devcap                data.devcap
#define d_turbo                 data.turbo
#define d_param                 data.param
#define d_optie                 data.optie
#define d_appie                 data.appie
#define d_filter                data.filter
#define d_addba                 data.addba
#define d_delba                 data.delba
#define d_addba_status          data.addba_status
#define d_addba_resp            data.addba_resp
#define d_key                   data.key
#define d_mlme                  data.mlme
#define d_chanlist              data.chanlist
#define d_chaninfo              data.chaninfo
#define d_wmmparam              data.wmmparam
#define d_wpaie                 data.wpaie
#define d_wscie                 data.wscie
#define d_power                 data.power
#define d_station               data.station
#define d_range                 data.range
#define d_stastats              data.stats_sta
#define d_vapstats              data.stats_vap
#define d_devstats              data.stats_dev
#define d_phystats              data.stats_phy
#define d_hststats              data.stats_hst
#define d_tgtstats              data.stats_tgt
#define d_diag                  data.dev_diag
#define d_phyerr                data.phyerr
#define d_cwm                   data.cwm
#define d_ethtool               data.ethtool
#define d_vapinfo               data.vap_info
#define d_mtu                   data.mtu
#define d_mac                   data.mac
#define d_datum                 data.datum
#define d_dbginfo               data.dbg_info
#define d_reg                   data.reg
#define d_pktlog                data.pktlog_eventlist
#define d_size                  data.size
#define d_pktlog_read           data.pktlog_read_info
#define d_icscandelay           data.ic_scandelay
#define d_autoassoc             data.ic_autoassoc
#define d_productinfo           data.product_info
#define d_radio                 data.radio
#define d_spec_chan_scan        data.ic_specchanscan
#define d_hiddenssid_scan       data.ic_hiddenssidscan
#define d_tx99_rate			data.tx99.rate
#define	d_tx99_freq			data.tx99.freq
#define	d_tx99_htmode		data.tx99.htmode
#define	d_tx99_htext		data.tx99.htext
#define	d_tx99_power		data.tx99.power
#define	d_tx99_txmode		data.tx99.txmode
#define	d_tx99_type			data.tx99.type
#define	d_tx99_chanmask		data.tx99.chanmask
#define	d_tx99_rcvmode		data.tx99.rcvmode
#define d_htrates               data.htrates
#define d_wmi_timeout           data.wmi_stats_timeout
                                

#ifdef OS_TYPE_LINUX
    #define SIOCGADFWIOCTL  SIOCDEVPRIVATE+1
    #define SIOCSADFWIOCTL  SIOCDEVPRIVATE+2
#else
    #define SIOCGADFWIOCTL  _IOWR('i', 237, athcfg_wcmd_t)
    #define SIOCSADFWIOCTL  _IOW('i', 238, athcfg_wcmd_t)
#endif


#endif /* _ADF_WIOCTL_PVT_H */

/**
 * @}
 */ 














