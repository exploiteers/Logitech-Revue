/*
 * WPA Supplicant - driver interaction with MADWIFI 802.11 driver
 * Copyright (c) 2004, Sam Leffler <sam@errno.com>
 * Copyright (c) 2004-2005, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"
#include <sys/ioctl.h>

#include "common.h"
#include "driver.h"
#include "driver_wext.h"
#include "eloop.h"
#include "wpa_supplicant.h"
#include "wpa.h"
#include "wireless_copy.h"
#include <utils/Log.h>

#define OS_TYPE_LINUX

/* substitute for "driver_adf.h" */
#include "athcfg_wcmd.h"

/******************************************************************
	The below definitions should be available in "athcfg_wcmd.h"
******************************************************************/
/*
 * Authentication mode.
 */
enum ieee80211_authmode {
    IEEE80211_AUTH_NONE = 0,
    IEEE80211_AUTH_OPEN = 1,        /* open */
    IEEE80211_AUTH_SHARED   = 2,    /* shared-key */
    IEEE80211_AUTH_8021X    = 3,    /* 802.1x */
    IEEE80211_AUTH_AUTO = 4,		/* auto-select/accept */
    /* NB: these are used only for ioctls */
    IEEE80211_AUTH_WPA  = 5,        /* WPA/RSN w/ 802.1x/PSK */
};

#ifdef IEEE80211_IOCTL_SETWMMPARAMS
/* Assume this is built against madwifi-ng */
#define MADWIFI_NG
#endif /* IEEE80211_IOCTL_SETWMMPARAMS */

#ifdef EAP_WPS
#ifndef USE_INTEL_SDK
#include "wps_config.h"
#endif /* USE_INTEL_SDK */
#endif /* EAP_WPS */

struct wpa_driver_madwifi_data {
	void *wext; /* private data for driver_wext */
	void *ctx;
	char ifname[IFNAMSIZ + 1];
	int sock;
#ifdef CONFIG_ETHERSTA
    unsigned char own_addr[6];
#endif /* CONFIG_ETHERSTA */
};

static int
adf_do_ioctl(int s, int number, athcfg_wcmd_t *i_req)
{
    /* Try to open the socket, if success returns it */
#ifdef OS_TYPE_LINUX
    struct ifreq ifr;
    strcpy(ifr.ifr_name, i_req->if_name);

    ifr.ifr_data = (void *)i_req;
    if ((ioctl (s,number, &ifr)) !=0) {
#else
    if ((ioctl (s,number, i_req)) !=0) {
#endif
        fprintf(stderr, "Error doing ioctl(): %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int
set80211priv_adf_setkey(struct wpa_driver_madwifi_data *drv, int op, void *data, int len)
{
    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(struct athcfg_wcmd));

    athcfg_wcmd_param_t param; 
    memset(&param, 0, sizeof(struct athcfg_wcmd_param));

    athcfg_wcmd_keyinfo_t key; 
    memset(&key, 0, sizeof(struct athcfg_wcmd_keyinfo));

    athcfg_wcmd_keyinfo_t * keyptr = data;

    strcpy(i_req.if_name, drv->ifname);

	/* Set the privacy bit before setting the Key */
	/* Run the set Param command to set the privacy bit */

	param.value = 1;
    param.param_id = ATHCFG_WCMD_PARAM_PRIVACY; 
    i_req.type = ATHCFG_WCMD_SET_PARAM;
    i_req.d_param = param;

    if (adf_do_ioctl(drv->sock,SIOCSADFWIOCTL, &i_req) < 0)
		return EIO;

	key.ik_type 	= keyptr->ik_type;
	key.ik_keyix 	= keyptr->ik_keyix;
	key.ik_keylen 	= keyptr->ik_keylen;
	key.ik_flags 	= keyptr->ik_flags;
	memcpy( key.ik_macaddr, keyptr->ik_macaddr, ATHCFG_WCMD_ADDR_LEN);
	memcpy( key.ik_keydata, keyptr->ik_keydata, key.ik_keylen);
    
	i_req.type = ATHCFG_WCMD_SET_KEY;
	//i_req.len = len; /* TODO::: Need to check the input */
	i_req.d_key = key;

   	if (adf_do_ioctl(drv->sock,SIOCSADFWIOCTL, &i_req) < 0) {
		int err = errno;
		perror("set80211priv_adf_setkey ioctl failed");
		wpa_printf(MSG_ERROR, "ioctl 0x%x failed errno=%d", op, err);
   		return EIO;
	}

	return 0;
}

static int
set80211priv_adf_delkey(struct wpa_driver_madwifi_data *drv, int op, void *data, int len)
{
    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(struct athcfg_wcmd));

    athcfg_wcmd_keyinfo_t key; 
    memset(&key, 0, sizeof(struct athcfg_wcmd_keyinfo));

    athcfg_wcmd_keyinfo_t * keyptr = data;

    strcpy(i_req.if_name, drv->ifname);

	key.ik_keyix 	= keyptr->ik_keyix;
	memcpy( key.ik_macaddr, keyptr->ik_macaddr, ATHCFG_WCMD_ADDR_LEN);

	i_req.type = ATHCFG_WCMD_SET_DELKEY;
	//i_req.len = len; /* TODO::: Need to check the input */
	i_req.d_key = key;

	if (adf_do_ioctl(drv->sock,SIOCSADFWIOCTL, &i_req) < 0) {
		int err = errno;
		perror("set80211priv_adf_delkey ioctl failed");
		wpa_printf(MSG_ERROR, "ioctl 0x%x failed errno=%d", op, err);
   		return EIO;
	}

	return 0;
}

static int
set80211priv_adf_mlme(struct wpa_driver_madwifi_data *drv, int op, void *data, int len)
{

    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(struct athcfg_wcmd));
 
    athcfg_wcmd_mlme_t mlme;
    memset(&mlme, 0, sizeof(struct athcfg_wcmd_mlme));

    athcfg_wcmd_mlme_t * mlmeptr = data;
    strcpy(i_req.if_name, drv->ifname);

	mlme.op = mlmeptr->op;
	mlme.reason = mlmeptr->reason;
	memcpy(mlme.mac.addr, mlmeptr->mac.addr, ATHCFG_WCMD_ADDR_LEN);
	i_req.type = ATHCFG_WCMD_SET_MLME;
	//i_req.len = len; /* TODO::: Need to check the input */
	i_req.d_mlme = mlme;

	if (adf_do_ioctl(drv->sock, SIOCSADFWIOCTL, &i_req) < 0) {
		int err = errno;
		perror("set80211priv_adf_mlme ioctl failed");
		wpa_printf(MSG_ERROR, "ioctl 0x%x failed errno=%d", op, err);
   		return EIO;
	}

	return 0;
}

#if 0
static int
set80211priv_adf_appie(struct wpa_driver_madwifi_data *drv, int op, void *data, int len)
{

    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(struct athcfg_wcmd));
 
    athcfg_wcmd_appie_t appie;
    memset(&appie, 0, sizeof(struct athcfg_wcmd_appie));

    athcfg_wcmd_appie_t * appieptr = data;
    strcpy(i_req.if_name, drv->ifname);

	appie.frmtype = appieptr->frmtype;
	appie.len = appieptr->len;
	memcpy(appie.data, appieptr->data, appieptr->len);
		
	i_req.type = ATHCFG_WCMD_SET_APP_IE_BUF;
	//i_req.len = len; /* TODO::: Need to check the input */
	i_req.d_appie = appie;

	if (adf_do_ioctl(drv->sock, SIOCSADFWIOCTL, &i_req) < 0) {
		int err = errno;
		perror("set80211priv_adf_appie ioctl failed");
		wpa_printf(MSG_ERROR, "ioctl 0x%x failed errno=%d", op, err);
   		return EIO;
	}

	return 0;
}
#endif

static int
set80211param(struct wpa_driver_madwifi_data *drv, int op, int arg)
{
    athcfg_wcmd_t i_req;
    memset(&i_req,0, sizeof(struct athcfg_wcmd));

    athcfg_wcmd_param_t param;
    memset(&param, 0, sizeof(struct athcfg_wcmd_param));

    strncpy(i_req.if_name, drv->ifname, IFNAMSIZ);

    param.param_id = op;
    param.value = arg;
    i_req.type = ATHCFG_WCMD_SET_PARAM;
	//i_req.len = sizeof(athcfg_wcmd_t); /* TODO::: Need to check the input */
    i_req.d_param = param;

    if (adf_do_ioctl(drv->sock, SIOCSADFWIOCTL, &i_req) < 0) {
        perror("ioctl[SIOCSADFWIOCTL]");
        wpa_printf(MSG_DEBUG, "%s: Failed to set parameter (op %d "
               "arg %d)", __func__, op, arg);
        return -1;
    }
    return 0;
}


#ifdef MODIFIED_BY_SONY
static int
get80211param(struct wpa_driver_madwifi_data *drv, int op, int *arg,
	      int show_err)
{
	struct iwreq iwr;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.mode = op;

	if (ioctl(drv->sock, IEEE80211_IOCTL_GETPARAM, &iwr) < 0) {
		if (show_err) 
			perror("ioctl[IEEE80211_IOCTL_GETPARAM]");
		return -1;
	}
	os_memcpy(arg, &iwr.u.param.value, sizeof(arg));
	return 0;
}
#endif /* MODIFIED_BY_SONY */

static int
wpa_driver_madwifi_set_wpa_ie(struct wpa_driver_madwifi_data *drv,
			      const u8 *wpa_ie, size_t wpa_ie_len)
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_optie_t optie;
    
    os_memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    os_memset(&optie, 0, sizeof(athcfg_wcmd_optie_t));
    
{
    int i;
    for (i=0; i<wpa_ie_len; i++) {
        printf("%x ", wpa_ie[i]);
    }
    printf("\n");
}
    os_memcpy(optie.data, wpa_ie, wpa_ie_len);
    optie.len = wpa_ie_len;

    os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_SET_OPT_IE;
    i_req.d_optie = optie;

    if (adf_do_ioctl(drv->sock, SIOCSADFWIOCTL, &i_req) < 0) {
        perror("Failed to set wpa ie");        
        return -1;
    }

    return 0;
    
}

static int
wpa_driver_madwifi_del_key(struct wpa_driver_madwifi_data *drv, int key_idx,
			   const u8 *addr)
{
	athcfg_wcmd_keyinfo_t wk;

	wpa_printf(MSG_DEBUG, "%s: keyidx=%d", __FUNCTION__, key_idx);
	os_memset(&wk, 0, sizeof(wk));
	wk.ik_keyix = key_idx;
	if (addr != NULL)
		os_memcpy(wk.ik_macaddr, addr, ATHCFG_WCMD_ADDR_LEN);

	return set80211priv_adf_delkey(drv, ATHCFG_WCMD_SET_DELKEY, &wk, sizeof(wk));
}

static int
wpa_driver_madwifi_set_key(void *priv, wpa_alg alg,
			   const u8 *addr, int key_idx, int set_tx,
			   const u8 *seq, size_t seq_len,
			   const u8 *key, size_t key_len)
{
	struct wpa_driver_madwifi_data *drv = priv;
	athcfg_wcmd_keyinfo_t wk;
	char *alg_name;
	u_int8_t cipher;

    wpa_printf(MSG_WARNING, "%s", __func__);
	if (alg == WPA_ALG_NONE)
		return wpa_driver_madwifi_del_key(drv, key_idx, addr);

	switch (alg) {
	case WPA_ALG_WEP:

		if (addr == NULL || os_memcmp(addr, "\xff\xff\xff\xff\xff\xff",
					      ETH_ALEN) == 0) {
#if 0
            wpa_printf(MSG_INFO, "WEP not supported now\n");
#else
			/*
			 * madwifi did not seem to like static WEP key
			 * configuration with IEEE80211_IOCTL_SETKEY, so use
			 * Linux wireless extensions ioctl for this.
			 */
			return wpa_driver_wext_set_key(drv->wext, alg, addr,
						       key_idx, set_tx,
						       seq, seq_len,
						       key, key_len);
#endif
		}

		alg_name = "WEP";
		cipher = ATHCFG_WCMD_CIPHERMODE_WEP;
		break;
	case WPA_ALG_TKIP:
		alg_name = "TKIP";
		cipher = ATHCFG_WCMD_CIPHERMODE_TKIP;
		break;
	case WPA_ALG_CCMP:
		alg_name = "CCMP";
		cipher = ATHCFG_WCMD_CIPHERMODE_AES_CCM;
		break;
	default:
		wpa_printf(MSG_DEBUG, "%s: unknown/unsupported algorithm %d",
			__FUNCTION__, alg);
		return -1;
	}

	wpa_printf(MSG_DEBUG, "%s: alg=%s key_idx=%d set_tx=%d seq_len=%lu "
		   "key_len=%lu", __FUNCTION__, alg_name, key_idx, set_tx,
		   (unsigned long) seq_len, (unsigned long) key_len);

	if (seq_len > sizeof(u_int64_t)) {
		wpa_printf(MSG_DEBUG, "%s: seq_len %lu too big",
			   __FUNCTION__, (unsigned long) seq_len);
		return -2;
	}
	if (key_len > sizeof(wk.ik_keydata)) {
		wpa_printf(MSG_DEBUG, "%s: key length %lu too big",
			   __FUNCTION__, (unsigned long) key_len);
		return -3;
	}

	os_memset(&wk, 0, sizeof(wk));
	wk.ik_type = cipher;
	wk.ik_flags = ATHCFG_WCMD_VAPKEY_RECV;
	if (addr == NULL ||
	    os_memcmp(addr, "\xff\xff\xff\xff\xff\xff", ETH_ALEN) == 0)
		wk.ik_flags |= ATHCFG_WCMD_VAPKEY_GROUP;
	if (set_tx) {
		wk.ik_flags |= ATHCFG_WCMD_VAPKEY_XMIT | ATHCFG_WCMD_VAPKEY_DEFAULT;
		os_memcpy(wk.ik_macaddr, addr, ATHCFG_WCMD_ADDR_LEN);
	} else
		os_memset(wk.ik_macaddr, 0, ATHCFG_WCMD_ADDR_LEN);
	wk.ik_keyix = key_idx;
	wk.ik_keylen = key_len;
#ifdef WORDS_BIGENDIAN
#define WPA_KEY_RSC_LEN 8
	{
		size_t i;
		u8 tmp[WPA_KEY_RSC_LEN];
		os_memset(tmp, 0, sizeof(tmp));
		for (i = 0; i < seq_len; i++)
			tmp[WPA_KEY_RSC_LEN - i - 1] = seq[i];
		os_memcpy(&wk.ik_keyrsc, tmp, WPA_KEY_RSC_LEN);
	}
#else /* WORDS_BIGENDIAN */
	os_memcpy(&wk.ik_keyrsc, seq, seq_len);
#endif /* WORDS_BIGENDIAN */
	os_memcpy(wk.ik_keydata, key, key_len);

	return set80211priv_adf_setkey(drv, ATHCFG_WCMD_SET_KEY, &wk, sizeof(wk));
}

static int
wpa_driver_madwifi_set_countermeasures(void *priv, int enabled)
{
	struct wpa_driver_madwifi_data *drv = priv;
    wpa_printf(MSG_WARNING, "%s", __func__);
	wpa_printf(MSG_DEBUG, "%s: enabled=%d", __FUNCTION__, enabled);
	return set80211param(drv, ATHCFG_WCMD_PARAM_COUNTERMEASURES, enabled);
}


static int
wpa_driver_madwifi_set_drop_unencrypted(void *priv, int enabled)
{
	struct wpa_driver_madwifi_data *drv = priv;
    wpa_printf(MSG_WARNING, "%s", __func__);
	wpa_printf(MSG_DEBUG, "%s: enabled=%d", __FUNCTION__, enabled);
	return set80211param(drv, ATHCFG_WCMD_PARAM_DROPUNENCRYPTED, enabled);
}

static int
wpa_driver_madwifi_deauthenticate(void *priv, const u8 *addr, int reason_code)
{
	struct wpa_driver_madwifi_data *drv = priv;
	athcfg_wcmd_mlme_t mlme;

    wpa_printf(MSG_WARNING, "%s", __func__);
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	mlme.op = ATHCFG_WCMD_MLME_DEAUTH;
	mlme.reason = reason_code;
	os_memcpy(mlme.mac.addr, addr, ATHCFG_WCMD_ADDR_LEN);
	return set80211priv_adf_mlme(drv, ATHCFG_WCMD_SET_MLME, &mlme, sizeof(mlme));
}

static int
wpa_driver_madwifi_disassociate(void *priv, const u8 *addr, int reason_code)
{
	struct wpa_driver_madwifi_data *drv = priv;
	athcfg_wcmd_mlme_t mlme;

    wpa_printf(MSG_WARNING, "%s", __func__);
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	mlme.op = ATHCFG_WCMD_MLME_DISASSOC;
	mlme.reason = reason_code;
	os_memcpy(mlme.mac.addr, addr, ATHCFG_WCMD_ADDR_LEN);
	return set80211priv_adf_mlme(drv, ATHCFG_WCMD_SET_MLME, &mlme, sizeof(mlme));
}

static int
madwifi_set_ssid(const char *ifname, void *priv, const u8 *buf, int len)
{
	struct wpa_driver_madwifi_data *drv = priv;
    athcfg_wcmd_t i_req ;
    memset(&i_req,0, sizeof(struct athcfg_wcmd));

    athcfg_wcmd_ssid_t essid; 
    memset(&essid, 0, sizeof(struct athcfg_wcmd_ssid));

    if (strlen(ifname) > sizeof(i_req.if_name)) {
        fprintf(stderr, "Ifname too big %s!\n", ifname);
        return E2BIG;
    }

    if( (strlen((const char *)buf) <= ATHCFG_WCMD_MAX_SSID) )
        strcpy((char *)essid.byte, (const char*)buf);

    essid.len = strlen((const char *)buf);	

    strcpy(i_req.if_name, ifname);
    i_req.type = ATHCFG_WCMD_SET_ESSID;
    i_req.d_essid = essid;
	//i_req.len = essid.len;

    if (adf_do_ioctl(drv->sock, SIOCSADFWIOCTL, &i_req) < 0)
        return EIO;

	return 0;
}

static int
wpa_driver_madwifi_associate(void *priv,
			     struct wpa_driver_associate_params *params)
{
	struct wpa_driver_madwifi_data *drv = priv;
	athcfg_wcmd_mlme_t mlme;
	int ret = 0, privacy = 1;

    wpa_printf(MSG_WARNING, "%s", __func__);
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	/*
	 * If the device was earlier connected to a different accesspoint,
	 * then Android will not send a dissassociate to this AP before making a new 
	 * association. So we need to signal the Network state tracker that
	 * we got disconnected from earlier AP, so that it can flush the 
	 * DHCP and DNS addresses, and initiate new requests for the new AP.
	 * If there is no existing association, then network state tracker 
	 * will ignore this event.
	 */
	wpa_supplicant_event(drv->ctx, EVENT_DISASSOC, NULL);
#ifdef MODIFIED_BY_SONY
		(void)wpa_driver_wext_auth_alg_fallback(drv->wext, params);
#endif /* MODIFIED_BY_SONY */

	/*
	 * NB: Don't need to set the freq or cipher-related state as
	 *     this is implied by the bssid which is used to locate
	 *     the scanned node state which holds it.  The ssid is
	 *     needed to disambiguate an AP that broadcasts multiple
	 *     ssid's but uses the same bssid.
	 */
	/* XXX error handling is wrong but unclear what to do... */
        printf("wpa_s: params->wpa_ie_len: %u\n", params->wpa_ie_len);
	if (wpa_driver_madwifi_set_wpa_ie(drv, params->wpa_ie,
					  params->wpa_ie_len) < 0) {
                printf("wpa_driver_madwifi_set_wpa_ie failed\n");
		ret = -1;
        }

	if (params->pairwise_suite == CIPHER_NONE &&
	    params->group_suite == CIPHER_NONE &&
	    params->key_mgmt_suite == KEY_MGMT_NONE &&
	    params->wpa_ie_len == 0)
		privacy = 0;

	if (set80211param(drv, ATHCFG_WCMD_PARAM_PRIVACY, privacy) < 0)
		ret = -1;

	if (params->wpa_ie_len &&
	    set80211param(drv, ATHCFG_WCMD_PARAM_WPA,
			  params->wpa_ie[0] == RSN_INFO_ELEM ? 2 : 1) < 0)
		ret = -1;

	if (params->bssid == NULL) {
		/* ap_scan=2 mode - driver takes care of AP selection and
		 * roaming */
		/* FIX: this does not seem to work; would probably need to
		 * change something in the driver */
		if (set80211param(drv, ATHCFG_WCMD_PARAM_ROAMING, 1) < 0)
			ret = -1;

        if (madwifi_set_ssid(drv->ifname, drv, params->ssid, params->ssid_len) < 0)
            ret = -1;

	} else {
		if (set80211param(drv, ATHCFG_WCMD_PARAM_ROAMING, 2) < 0)
			ret = -1;

        if (madwifi_set_ssid(drv->ifname, drv, params->ssid, params->ssid_len) < 0)
            ret = -1;

		os_memset(&mlme, 0, sizeof(mlme));
		mlme.op = ATHCFG_WCMD_MLME_ASSOC;
		os_memcpy(mlme.mac.addr, params->bssid, ATHCFG_WCMD_ADDR_LEN);
		if (set80211priv_adf_mlme(drv, ATHCFG_WCMD_SET_MLME, &mlme,
				 sizeof(mlme)) < 0) {
			wpa_printf(MSG_DEBUG, "%s: SETMLME[ASSOC] failed",
				   __func__);
			ret = -1;
		}
	}

	return ret;
}

static int
wpa_driver_madwifi_set_auth_alg(void *priv, int auth_alg)
{
	struct wpa_driver_madwifi_data *drv = priv;
	int authmode;

    wpa_printf(MSG_WARNING, "%s", __func__);
	if ((auth_alg & AUTH_ALG_OPEN_SYSTEM) &&
	    (auth_alg & AUTH_ALG_SHARED_KEY))
		authmode = IEEE80211_AUTH_AUTO;
	else if (auth_alg & AUTH_ALG_SHARED_KEY)
		authmode = IEEE80211_AUTH_SHARED;
	else
		authmode = IEEE80211_AUTH_OPEN;

	return set80211param(drv, ATHCFG_WCMD_PARAM_AUTHMODE, authmode);
}

/**
 * scan_timeout - Scan timeout to report scan completion
 * @eloop_ctx: Unused
 * @timeout_ctx: ctx argument given to wpa_driver_wext_init()
 *
 * This function can be used as registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
void scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	wpa_printf(MSG_DEBUG, "Scan timeout - try to get results");
	wpa_supplicant_event(timeout_ctx, EVENT_SCAN_RESULTS, NULL);
}

static int
wpa_driver_madwifi_scan(void *priv, const u8 *ssid, size_t ssid_len)
{
    struct wpa_driver_madwifi_data *drv = priv;
    athcfg_wcmd_t i_req;
    int ret = 0;

    wpa_printf(MSG_WARNING, "%s", __func__);
    os_memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));

    if (ssid != NULL) {
        if (madwifi_set_ssid (drv->ifname, drv, ssid, ssid_len) < 0)
            ret = -1;
    }

    i_req.type = ATHCFG_WCMD_SET_SCAN;
    if (adf_do_ioctl(drv->sock, SIOCSADFWIOCTL, &i_req) < 0) {
        perror("wpa_driver_madwifi_scan ioctl failed."); 	   
        return -1;
    }

	/*
	 * madwifi delivers a scan complete event so no need to poll, but
	 * register a backup timeout anyway to make sure that we recover even
	 * if the driver does not send this event for any reason. This timeout
	 * will only be used if the event is not delivered (event handler will
	 * cancel the timeout).
	 */
	eloop_register_timeout(60, 0, scan_timeout, drv->wext,
			       drv->ctx);

	return ret;
}

static int wpa_driver_madwifi_get_bssid(void *priv, u8 *bssid)
{
	struct wpa_driver_madwifi_data *drv = priv;
    athcfg_wcmd_t i_req;
    athcfg_wcmd_bssid_t ath_bssid;
    int ret = 0;

    wpa_printf(MSG_WARNING, "%s", __func__);
    os_memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    os_memset(&ath_bssid, 0, sizeof(athcfg_wcmd_bssid_t));
    os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_GET_BSSID;
    i_req.d_bssid = ath_bssid;
    if (adf_do_ioctl(drv->sock, SIOCGADFWIOCTL, &i_req) < 0) {
        perror("wpa_driver_madwifi_get_bssid ioctl failed.");
        ret = -1;
    }
    os_memcpy(bssid, i_req.d_bssid.bssid, ETH_ALEN);

printf("wpa_s: bssid - %x:%x:%x:%x:%x:%x\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return 0;
}


static int wpa_driver_madwifi_get_ssid(void *priv, u8 *ssid)
{
	struct wpa_driver_madwifi_data *drv = priv;
    athcfg_wcmd_t i_req;
    athcfg_wcmd_ssid_t essid; 

    wpa_printf(MSG_WARNING, "%s", __func__);
    os_memset(&i_req, 0, sizeof(struct athcfg_wcmd));
    os_memset(&essid, 0, sizeof(struct athcfg_wcmd_ssid));
    os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_GET_ESSID;
    i_req.d_essid = essid;

    if (adf_do_ioctl(drv->sock, SIOCGADFWIOCTL, &i_req) < 0) {
        perror("wpa_driver_madwifi_get_ssid ioctl failed.");
	    return -1;
	}

//printf("%s, len: %u\n", i_req.d_essid.byte, i_req.d_essid.len);
    os_strncpy(((char *)ssid), ((char *)i_req.d_essid.byte), i_req.d_essid.len);
//printf("%s\n", ssid);
    return i_req.d_essid.len;
}

/* Compare function for sorting scan results. Return >0 if @b is considered
 * better. */
static int wpa_scan_result_compar(const void *a, const void *b)
{
	const struct wpa_scan_result *wa = a;
	const struct wpa_scan_result *wb = b;

	/* WPA/WPA2 support preferred */
	if ((wb->wpa_ie_len || wb->rsn_ie_len) &&
	    !(wa->wpa_ie_len || wa->rsn_ie_len))
		return 1;
	if (!(wb->wpa_ie_len || wb->rsn_ie_len) &&
	    (wa->wpa_ie_len || wa->rsn_ie_len))
		return -1;

	/* privacy support preferred */
	if ((wa->caps & IEEE80211_CAP_PRIVACY) == 0 &&
	    (wb->caps & IEEE80211_CAP_PRIVACY))
		return 1;
	if ((wa->caps & IEEE80211_CAP_PRIVACY) &&
	    (wb->caps & IEEE80211_CAP_PRIVACY) == 0)
		return -1;

	/* best/max rate preferred if signal level close enough XXX */
	if (wa->maxrate != wb->maxrate && abs(wb->level - wa->level) < 5)
		return wb->maxrate - wa->maxrate;

	/* use freq for channel preference */

	/* all things being equal, use signal level; if signal levels are
	 * identical, use quality values since some drivers may only report
	 * that value and leave the signal level zero */
	if (wb->level == wa->level)
		return wb->qual - wa->qual;
	return wb->level - wa->level;
}

static int wpa_driver_madwifi_get_scan_results(void *priv,
					    struct wpa_scan_result *results,
					    size_t max_size)
{
	struct wpa_driver_madwifi_data *drv = priv;
	athcfg_wcmd_t i_req;
	athcfg_wcmd_scan_t scan;
	size_t ap_num = 0;
    a_uint8_t offset = 0, count;

	os_memset(&i_req, 0, sizeof(struct athcfg_wcmd));
	os_memset(&scan, 0, sizeof(struct athcfg_wcmd_scan));
	os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));
	i_req.type = ATHCFG_WCMD_GET_SCAN;
	i_req.d_scan = &scan;

    os_memset(results, 0, max_size * sizeof(struct wpa_scan_result));

    printf("max_size: %u\n", max_size);

    do 
    {
	if (adf_do_ioctl(drv->sock, SIOCGADFWIOCTL, &i_req) < 0) {
	    perror("wpa_driver_madwifi_get_scan_results ioctl failed.");
	    return -1;
	}

        offset += i_req.d_scan->len;
        scan.offset = offset;

        printf("offset: %u, i_req.d_scan->len: %u, ap_num:%u\n", offset, i_req.d_scan->len, ap_num);
	if (i_req.d_scan->len) {

            for (count = 0; ap_num < offset; ap_num++, count++) {
                if (ap_num >= max_size) {
                    wpa_printf(MSG_DEBUG, "Too small scan result buffer - "
                                          "%lu BSSes but room only for %lu",
                                          (unsigned long) offset,
                                          (unsigned long) max_size);
                    ap_num = max_size;
                    break;
                }

	        /* BSSID */
	        os_memcpy(results[ap_num].bssid, i_req.d_scan->result[count].isr_bssid, ETH_ALEN);

	        /* SSID */
	        results[ap_num].ssid_len = i_req.d_scan->result[count].isr_ssid_len;
	        os_memcpy(results[ap_num].ssid, 
                          i_req.d_scan->result[count].isr_ssid, 
                          results[ap_num].ssid_len);

	        /* WPA IE */
	        results[ap_num].wpa_ie_len = i_req.d_scan->result[count].isr_wpa_ie.len;
	        os_memcpy(results[ap_num].wpa_ie, 
                          i_req.d_scan->result[count].isr_wpa_ie.data,
                          results[ap_num].wpa_ie_len);

            /* RSN IE */
            results[ap_num].rsn_ie_len = i_req.d_scan->result[count].isr_rsn_ie.len;
            os_memcpy(results[ap_num].rsn_ie,
                      i_req.d_scan->result[count].isr_rsn_ie.data,
                      results[ap_num].rsn_ie_len);

#ifdef EAP_WPS
            /* WPS IE */
            results[ap_num].wps_ie_len = i_req.d_scan->result[count].isr_wps_ie.len;
            os_memcpy(results[ap_num].wps_ie,
                      i_req.d_scan->result[count].isr_wps_ie.data,
                      results[ap_num].wps_ie_len);
#endif

            /* frequency */
            results[ap_num].freq = i_req.d_scan->result[count].isr_freq;

            /* capability */
            results[ap_num].caps = i_req.d_scan->result[count].isr_capinfo;

            /* signal quality */
            results[ap_num].qual = i_req.d_scan->result[count].isr_rssi;
            
            /* noise level */
            results[ap_num].noise = 161;

            /* signal level */
            results[ap_num].level = results[ap_num].noise + results[ap_num].qual;

            /* maximum supported rate */
            results[ap_num].maxrate = i_req.d_scan->result[count].isr_rates[0];
            
	    }
	    
	}
    } while (i_req.d_scan->more);

    wpa_printf(MSG_WARNING, "%s #6: apnum: %u", __func__, ap_num);
    qsort(results, ap_num, sizeof(struct wpa_scan_result),
          wpa_scan_result_compar);

    wpa_printf(MSG_WARNING, "Received scan results (%lu BSSes)", (unsigned long) ap_num);

	return ap_num;
	
}

#if 0
static int wpa_driver_madwifi_set_operstate(void *priv, int state)
{
    wpa_printf(MSG_WARNING, "Setting operstate is not yet supported.");
    return -1;
#if 0
	struct wpa_driver_madwifi_data *drv = priv;
	return wpa_driver_wext_set_operstate(drv->wext, state);
#endif
}
#endif

#if 0
#ifdef EAP_WPS
#ifndef USE_INTEL_SDK
static int
madwifi_set_wps_ie(void *priv, u8 *iebuf, int iebuflen, u32 frametype)
{
#if 1//shliu: 0219
        wpa_printf(MSG_WARNING, "%s not support\n", __func__);
	return -1;
#else
	u8 buf[256];
	struct ieee80211req_getset_appiebuf * beac_ie;
	// int i;

	wpa_printf(MSG_DEBUG, "%s buflen = %d\n", __func__, iebuflen);

	beac_ie = (struct ieee80211req_getset_appiebuf *) buf;
	beac_ie->app_frmtype = frametype;
	beac_ie->app_buflen = iebuflen;
	os_memcpy(&(beac_ie->app_buf[0]), iebuf, iebuflen);
	
	return set80211priv(priv, IEEE80211_IOCTL_SET_APPIEBUF, beac_ie,
			sizeof(struct ieee80211req_getset_appiebuf) + iebuflen, 1);
#endif
}


static int wpa_driver_madwifi_set_wps_probe_req_ie(
			     void *priv, u8 *iebuf, int iebuflen)
{
#if 1//shliu: 0219
        wpa_printf(MSG_WARNING, "%s not support\n", __func__);
	return -1;
#else
	return madwifi_set_wps_ie(priv, iebuf, iebuflen, 
			IEEE80211_APPIE_FRAME_PROBE_REQ);
#endif
}


static int wpa_driver_madwifi_set_wps_assoc_req_ie(
			     void *priv, u8 *iebuf, int iebuflen)
{
#if 1//shliu: 0219
        wpa_printf(MSG_WARNING, "%s not support\n", __func__);
	return -1;
#else
	return madwifi_set_wps_ie(priv, iebuf, iebuflen, 
			IEEE80211_APPIE_FRAME_ASSOC_REQ);
#endif
}
#endif /* USE_INTEL_SDK */
#endif /* EAP_WPS */
#endif


static void * wpa_driver_madwifi_init(void *ctx, const char *ifname)
{
	struct wpa_driver_madwifi_data *drv;
#ifdef MODIFIED_BY_SONY
	int caps = 0;
#endif /* MODIFIED_BY_SONY */
#if 0
#ifdef EAP_WPS
#ifndef USE_INTEL_SDK
	int ret = -1;
#endif /* USE_INTEL_SDK */
#endif /* EAP_WPS */
#endif

    wpa_printf(MSG_WARNING, "%s", __func__);
	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;

	drv->wext = wpa_driver_wext_init(ctx, ifname);
	if (drv->wext == NULL)
		goto fail;

	drv->ctx = ctx;
	os_strncpy(drv->ifname, ifname, sizeof(drv->ifname));
	drv->sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (drv->sock < 0)
		goto fail2;

#ifdef MODIFIED_BY_SONY
	if (get80211param(drv, IEEE80211_PARAM_DRIVER_CAPS, &caps, 1) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to get driver capabilities"
			   "device_caps", __FUNCTION__);
	}
	wpa_printf(MSG_DEBUG, "driver capabilities : 0x%08X", caps);
#endif /* MODIFIED_BY_SONY */

#if 1 //shliu: 0223
	if (set80211param(drv, ATHCFG_WCMD_PARAM_ROAMING, 2) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to set wpa_supplicant-based "
			   "roaming", __FUNCTION__);
		goto fail3;
	}

	if (set80211param(drv, ATHCFG_WCMD_PARAM_WPA, 3) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to enable WPA support",
			   __FUNCTION__);
		goto fail3;
	}
#endif

#if 0//shliu: 0223
#ifdef EAP_WPS
#ifndef USE_INTEL_SDK
	do {
		/* Clear WPS ProbeResp IE */
		if (wpa_driver_madwifi_set_wps_probe_req_ie(drv, 0, 0)) {
			wpa_printf(MSG_DEBUG, "%s: failed to clear WPS ProbeReq IE",
				   __FUNCTION__);
			break;
		}
		/* Clear WPS AssocReq IE */
		if (wpa_driver_madwifi_set_wps_assoc_req_ie(drv, 0, 0)) {
			wpa_printf(MSG_DEBUG, "%s: failed to clear WPS AssocReq IE",
				   __FUNCTION__);
			break;
		}
		ret = 0;
	} while (0);

	if (ret)
		goto fail3;
#endif /* USE_INTEL_SDK */
#endif /* EAP_WPS */
#endif
	return drv;

fail3:
	close(drv->sock);
fail2:
	wpa_driver_wext_deinit(drv->wext);
fail:
	os_free(drv);
	return NULL;
}


static void wpa_driver_madwifi_deinit(void *priv)
{
	struct wpa_driver_madwifi_data *drv = priv;

    wpa_printf(MSG_WARNING, "%s", __func__);
	if (wpa_driver_madwifi_set_wpa_ie(drv, NULL, 0) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to clear WPA IE",
			   __FUNCTION__);
	}
	if (set80211param(drv, ATHCFG_WCMD_PARAM_ROAMING, 1) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to enable driver-based "
			   "roaming", __FUNCTION__);
	}
	if (set80211param(drv, ATHCFG_WCMD_PARAM_PRIVACY, 0) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to disable forced Privacy "
			   "flag", __FUNCTION__);
	}
	if (set80211param(drv, ATHCFG_WCMD_PARAM_WPA, 0) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed to disable WPA",
			   __FUNCTION__);
	}

	wpa_driver_wext_deinit(drv->wext);

	close(drv->sock);
	os_free(drv);
}

#ifdef CONFIG_ETHERSTA
/* This function is only used by ether-dongle mode */
static u8*
wpa_driver_madwifi_get_mac_addr(void *priv)
{
    struct wpa_driver_madwifi_data *drv = priv;
    athcfg_wcmd_t i_req;
    athcfg_ethaddr_t mac;

    os_memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    os_memset(&mac, 0, sizeof(athcfg_ethaddr_t));
    os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_GET_ETHER_DONGLE_MAC;
    i_req.d_mac = mac;
    if (adf_do_ioctl(drv->sock, SIOCGADFWIOCTL, &i_req) < 0) {
        perror("wpa_driver_madwifi_get_mac_addr failed.");
        return NULL;
    }     

    os_memcpy(drv->own_addr, i_req.d_mac.addr, sizeof(mac));

    return drv->own_addr;      
}
#endif /* CONFIG_ETHERSTA */

#ifdef ANDROID
int wpa_driver_madwifi_priv_driver_cmd ( void *priv, char *cmd, char *buf, size_t buf_len )
{
	struct wpa_driver_madwifi_data *drv = priv;
	struct ifreq ifr;
	struct iwreq wreq;
	unsigned char mac[6];
	athcfg_wcmd_t i_req;
	athcfg_wcmd_stainfo_t stainfo;
	unsigned int linkspeed = 0;

	if (os_strcasecmp(cmd, "RSSI-APPROX") == 0) {
		os_strncpy(cmd, "RSSI", MAX_DRV_CMD_SIZE);
	}
	if ((os_strcasecmp(cmd, "MACADDR") != 0) &&
		(os_strcasecmp(cmd, "RSSI") != 0)    &&
		(os_strcasecmp(cmd, "LINKSPEED") != 0)) {
		LOGE ("INVALID request to the wlan driver %s", cmd);
		return -1;
	}
	os_memset(&ifr, 0, sizeof(ifr));
	os_strncpy(ifr.ifr_name, drv->ifname, os_strlen(drv->ifname));

	if (os_strcasecmp(cmd, "MACADDR") == 0) {
		if (ioctl(drv->sock, SIOCGIFHWADDR, &ifr) < 0) {
			return -1;
		}
		memcpy (mac, ifr.ifr_addr.sa_data, 6);
		sprintf (buf, "Macaddr = %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		LOGI ("%s\n", buf);
	}
	else if (os_strcasecmp(cmd, "RSSI") == 0) {
		os_memset(&i_req, 0, sizeof(athcfg_wcmd_t));
		os_memset(&mac, 0, sizeof(athcfg_ethaddr_t));
		os_strncpy(i_req.if_name, drv->ifname, os_strlen(drv->ifname));

		i_req.type = ATHCFG_WCMD_GET_STATION_LIST;
		i_req.d_station = &stainfo;
		
		if (adf_do_ioctl(drv->sock, SIOCGADFWIOCTL, &i_req) < 0) {
			return -1;
		}     
		/*
		 * Units are in db above the noise floor. That means the
		 * rssi values reported in the tx/rx descriptors in the
		 * driver are the SNR expressed in db.
		 *
		 * If you assume that the noise floor is -95, which is an
		 * excellent assumption 99.5 % of the time, then you can
		 * derive the absolute signal level (i.e. -95 + rssi).
		 */
		sprintf (buf, "rssi %d\n", -95 + i_req.d_station->list[0].isi_rssi);
		//LOGI("adf_rssi %d", i_req.d_station->list[0].isi_rssi);
	}
	else if (os_strcasecmp(cmd, "LINKSPEED") == 0) {
		/*
		 * this api requires to run on wifi0 interface not ath0.
		 */
		char ifname[] = "wifi0";
		athcfg_wcmd_tgt_phystats_t tgt11n_phystats;

		strncpy(i_req.if_name, ifname, strlen(ifname));
		memset(&tgt11n_phystats, 0, sizeof(athcfg_wcmd_tgt_phystats_t));
		i_req.type = ATHCFG_WCMD_GET_DEV_TGT_11N_STATS;
	
		if (adf_do_ioctl(drv->sock, SIOCGADFWIOCTL, &i_req) < 0) {
			return -1;
		}     
		linkspeed = i_req.d_tgtstats->ast_11n_tgt.tx_rate/1000;
		//LOGI("linkspeed %d", linkspeed);
		sprintf (buf, "LinkSpeed %d\n", linkspeed);
	}
	return strlen(buf);
}
#endif
const struct wpa_driver_ops wpa_driver_adf_ops = {
	.name			= "madwifi",
	.desc			= "MADWIFI 802.11 support (Atheros, etc.)",
	.get_bssid		= wpa_driver_madwifi_get_bssid,
	.get_ssid		= wpa_driver_madwifi_get_ssid,
	.set_key		= wpa_driver_madwifi_set_key,
	.init			= wpa_driver_madwifi_init,
	.deinit			= wpa_driver_madwifi_deinit,
	.set_countermeasures	= wpa_driver_madwifi_set_countermeasures,
	.set_drop_unencrypted	= wpa_driver_madwifi_set_drop_unencrypted,
	.scan			= wpa_driver_madwifi_scan,
	.get_scan_results	= wpa_driver_madwifi_get_scan_results,
	.deauthenticate		= wpa_driver_madwifi_deauthenticate,
	.disassociate		= wpa_driver_madwifi_disassociate,
	.associate		= wpa_driver_madwifi_associate,
	.set_auth_alg		= wpa_driver_madwifi_set_auth_alg,
	.set_operstate		= NULL,//wpa_driver_madwifi_set_operstate,
#ifdef EAP_WPS
#ifndef USE_INTEL_SDK
	.set_wps_probe_req_ie	= NULL,//wpa_driver_madwifi_set_wps_probe_req_ie,
	.set_wps_assoc_req_ie	= NULL,//wpa_driver_madwifi_set_wps_assoc_req_ie,
#endif /* USE_INTEL_SDK */
#endif /* EAP_WPS */
#ifdef CONFIG_ETHERSTA
    .get_mac_addr           = wpa_driver_madwifi_get_mac_addr,
#endif /* CONFIG_ETHERSTA */
#ifdef ANDROID
	.driver_cmd             = wpa_driver_madwifi_priv_driver_cmd,
#endif
};
