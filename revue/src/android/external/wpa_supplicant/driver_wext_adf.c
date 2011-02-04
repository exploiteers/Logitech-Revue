/*
 * WPA Supplicant - driver interaction with generic Linux Wireless Extensions
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 *
 * This file implements a driver interface for the Linux Wireless Extensions.
 * When used with WE-18 or newer, this interface can be used as-is with number
 * of drivers. In addition to this, some of the common functions in this file
 * can be used by other driver interface implementations that use generic WE
 * ioctls, but require private ioctls for some of the functionality.
 */

#include "includes.h"
#include <sys/ioctl.h>
#include <net/if_arp.h>

#include "wireless_copy.h"
#include "common.h"
#include "driver.h"
#include "l2_packet.h"
#include "eloop.h"
#include "wpa_supplicant.h"
#include "priv_netlink.h"
#include "driver_wext.h"
#include "wpa.h"

#ifdef CONFIG_CLIENT_MLME
#include <netpacket/packet.h>
#include <hostapd_ioctl.h>
#include <ieee80211_common.h>
/* from net/mac80211.h */
enum {
	MODE_IEEE80211A = 0 /* IEEE 802.11a */,
	MODE_IEEE80211B = 1 /* IEEE 802.11b only */,
	MODE_ATHEROS_TURBO = 2 /* Atheros Turbo mode (2x.11a at 5 GHz) */,
	MODE_IEEE80211G = 3 /* IEEE 802.11g (and 802.11b compatibility) */,
	MODE_ATHEROS_TURBOG = 4 /* Atheros Turbo mode (2x.11g at 2.4 GHz) */,
	NUM_IEEE80211_MODES = 5
};

#include "mlme.h"

#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif
#endif /* CONFIG_CLIENT_MLME */

#ifdef CONFIG_DRIVER_ADF
#define OS_TYPE_LINUX
#include "athcfg_wcmd.h"
#endif /* CONFIG_DRIVER_ADF */

struct wpa_driver_wext_data {
	void *ctx;
	int event_sock;
	int ioctl_sock;
	int mlme_sock;
	char ifname[IFNAMSIZ + 1];
	int ifindex;
	int ifindex2;
	u8 *assoc_req_ies;
	size_t assoc_req_ies_len;
	u8 *assoc_resp_ies;
	size_t assoc_resp_ies_len;
	struct wpa_driver_capa capa;
	int has_capability;
	int we_version_compiled;

	/* for set_auth_alg fallback */
	int use_crypt;
	int auth_alg_fallback;

	int operstate;

	char mlmedev[IFNAMSIZ + 1];
};

void scan_timeout(void *eloop_ctx, void *timeout_ctx);

static int wpa_driver_wext_flush_pmkid(void *priv);
static int wpa_driver_wext_get_range(void *priv);


#ifdef CONFIG_DRIVER_ADF
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
#endif



static int wpa_driver_wext_send_oper_ifla(struct wpa_driver_wext_data *drv,
					  int linkmode, int operstate)
{
	struct {
		struct nlmsghdr hdr;
		struct ifinfomsg ifinfo;
		char opts[16];
	} req;
	struct rtattr *rta;
	static int nl_seq;
	ssize_t ret;

	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.hdr.nlmsg_type = RTM_SETLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST;
	req.hdr.nlmsg_seq = ++nl_seq;
	req.hdr.nlmsg_pid = 0;

	req.ifinfo.ifi_family = AF_UNSPEC;
	req.ifinfo.ifi_type = 0;
	req.ifinfo.ifi_index = drv->ifindex;
	req.ifinfo.ifi_flags = 0;
	req.ifinfo.ifi_change = 0;

	if (linkmode != -1) {
		rta = (struct rtattr *)
			((char *) &req + NLMSG_ALIGN(req.hdr.nlmsg_len));
		rta->rta_type = IFLA_LINKMODE;
		rta->rta_len = RTA_LENGTH(sizeof(char));
		*((char *) RTA_DATA(rta)) = linkmode;
		req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) +
			RTA_LENGTH(sizeof(char));
	}
	if (operstate != -1) {
		rta = (struct rtattr *)
			((char *) &req + NLMSG_ALIGN(req.hdr.nlmsg_len));
		rta->rta_type = IFLA_OPERSTATE;
		rta->rta_len = RTA_LENGTH(sizeof(char));
		*((char *) RTA_DATA(rta)) = operstate;
		req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) +
			RTA_LENGTH(sizeof(char));
	}

	wpa_printf(MSG_DEBUG, "WEXT: Operstate: linkmode=%d, operstate=%d",
		   linkmode, operstate);

	ret = send(drv->event_sock, &req, req.hdr.nlmsg_len, 0);
	if (ret < 0) {
		wpa_printf(MSG_DEBUG, "WEXT: Sending operstate IFLA failed: "
			   "%s (assume operstate is not supported)",
			   strerror(errno));
	}

	return ret < 0 ? -1 : 0;
}

#if 0
#ifdef CONFIG_DRIVER_ADF
static int
set80211param_adf(struct wpa_driver_wext_data *drv, int op, int arg,
	      int show_err)
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

    if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0) {
        perror("ioctl[SIOCSADFWIOCTL]");
        wpa_printf(MSG_DEBUG, "%s: Failed to set parameter (op %d "
               "arg %d)", __func__, op, arg);
        return -1;
    }
    return 0;
}
#endif /* CONFIG_DRIVER_ADF */
#endif


static int wpa_driver_wext_set_auth_param(struct wpa_driver_wext_data *drv,
					  int idx, u32 value)
{
#if 1
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.param.flags = idx & IW_AUTH_INDEX;
	iwr.u.param.value = value;

	if (ioctl(drv->ioctl_sock, SIOCSIWAUTH, &iwr) < 0) {
		perror("ioctl[SIOCSIWAUTH]");
		fprintf(stderr, "WEXT auth param %d value 0x%x - ",
			idx, value);
		ret = errno == EOPNOTSUPP ? -2 : -1;
	}
#endif

	return ret;
}


/**
 * wpa_driver_wext_get_bssid - Get BSSID, SIOCGIWAP
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @bssid: Buffer for BSSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_get_bssid(void *priv, u8 *bssid)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;

    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(athcfg_wcmd_t));

    athcfg_wcmd_bssid_t bssid_t; 
    memset(&bssid_t, 0, sizeof(athcfg_wcmd_bssid_t));

    strncpy(i_req.if_name, drv->ifname, strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_GET_BSSID;
    i_req.d_bssid = bssid_t;


    if (adf_do_ioctl(drv->ioctl_sock, SIOCGADFWIOCTL, &i_req) < 0)
        return EIO;
    
	os_memcpy(bssid, i_req.d_bssid.bssid, ATHCFG_ETH_LEN);
	return ret;
}


/**
 * wpa_driver_wext_set_bssid - Set BSSID, SIOCSIWAP
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @bssid: BSSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_bssid(void *priv, const u8 *bssid)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;

    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(athcfg_wcmd_t));

    athcfg_wcmd_bssid_t bssid_t; 
    memset(&bssid_t, 0, sizeof(athcfg_wcmd_bssid_t));

    strncpy(i_req.if_name, drv->ifname, strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_SET_BSSID;
    i_req.d_bssid = bssid_t;

	os_memcpy(i_req.d_bssid.bssid, bssid, ATHCFG_ETH_LEN);

    if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0)
        return EIO;
    
	return ret;
}


/**
 * wpa_driver_wext_get_ssid - Get SSID, SIOCGIWESSID
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: Buffer for the SSID; must be at least 32 bytes long
 * Returns: SSID length on success, -1 on failure
 */
int wpa_driver_wext_get_ssid(void *priv, u8 *ssid)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;

    athcfg_wcmd_t i_req;
    memset(&i_req,0, sizeof(struct athcfg_wcmd));

    athcfg_wcmd_ssid_t essid; 
    memset(&essid, 0, sizeof(struct athcfg_wcmd_ssid));

    strcpy(i_req.if_name, drv->ifname);
    i_req.type = ATHCFG_WCMD_GET_ESSID;
    i_req.d_essid = essid;

    if (adf_do_ioctl(drv->ioctl_sock, SIOCGADFWIOCTL, &i_req) < 0) {
		perror("ioctl[SIOCGIWESSID]");
		ret = -1;
	} else {
		ret = sizeof(i_req.d_essid.byte);
	    strncpy((char *)ssid, (char *)i_req.d_essid.byte, sizeof(i_req.d_essid.byte));
		if (ret > 32)
			ret = 32;
		/* Some drivers include nul termination in the SSID, so let's
		 * remove it here before further processing. WE-21 changes this
		 * to explicitly require the length _not_ to include nul
		 * termination. */
		if (ret > 0 && ssid[ret - 1] == '\0' &&
		    drv->we_version_compiled < 21)
			ret--;
	}

	return ret;
}


/**
 * wpa_driver_wext_set_ssid - Set SSID, SIOCSIWESSID
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: SSID
 * @ssid_len: Length of SSID (0..32)
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_ssid(void *priv, const u8 *ssid, size_t ssid_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;

    athcfg_wcmd_t i_req ;
    memset(&i_req,0, sizeof(struct athcfg_wcmd));

    athcfg_wcmd_ssid_t essid; 
    memset(&essid, 0, sizeof(struct athcfg_wcmd_ssid));

    if (sizeof(ssid) > sizeof(i_req.if_name)) {
        fprintf(stderr, "Ifname too big %s!\n", ssid);
        return E2BIG;
    }

    if( (ssid_len <= ATHCFG_WCMD_MAX_SSID) )
        strcpy((char *)essid.byte, (char *)ssid);

	if (drv->we_version_compiled < 21) {
		/* For historic reasons, set SSID length to include one extra
		 * character, C string nul termination, even though SSID is
		 * really an octet string that should not be presented as a C
		 * string. Some Linux drivers decrement the length by one and
		 * can thus end up missing the last octet of the SSID if the
		 * length is not incremented here. WE-21 changes this to
		 * explicitly require the length _not_ to include nul
		 * termination. */
		if (ssid_len)
			ssid_len++;
	}
    essid.len = ssid_len;	
	essid.flags = (ssid_len != 0);
	
    strcpy(i_req.if_name, drv->ifname);
    i_req.type = ATHCFG_WCMD_SET_ESSID;
    i_req.d_essid = essid;
	//i_req.len = essid.len;

    if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0)
        return EIO;


	return ret;
}


/**
 * wpa_driver_wext_set_freq - Set frequency/channel, SIOCSIWFREQ
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @freq: Frequency in MHz
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_freq(void *priv, int freq)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;

    athcfg_wcmd_t i_req; 
    memset(&i_req,0, sizeof(athcfg_wcmd_t));

    athcfg_wcmd_freq_t freq_t; 
    memset(&freq_t, 0, sizeof(athcfg_wcmd_freq_t));

    freq_t.m = freq * 100000;
    freq_t.e = 1;
    
    strcpy(i_req.if_name, drv->ifname);
    i_req.type = ATHCFG_WCMD_SET_FREQUENCY;
    i_req.d_freq = freq_t;

    if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0)
        return EIO;

	return ret;
}


static void
wpa_driver_wext_event_wireless_custom(void *ctx, char *custom)
{
	union wpa_event_data data;

	wpa_printf(MSG_MSGDUMP, "WEXT: Custom wireless event: '%s'",
		   custom);

	os_memset(&data, 0, sizeof(data));
	/* Host AP driver */
	if (os_strncmp(custom, "AUTH-FAILURE.indication", 23) == 0) {
		wpa_printf(MSG_DEBUG, "%s: recevied AUTH-FAILURE event\n",__func__);
		wpa_driver_wext_auth_failure_event_handler(ctx);
		return;
	} else if (os_strncmp(custom, "MLME-MICHAELMICFAILURE.indication", 33) == 0) {
		data.michael_mic_failure.unicast =
			os_strstr(custom, " unicast ") != NULL;
		/* TODO: parse parameters(?) */
		wpa_supplicant_event(ctx, EVENT_MICHAEL_MIC_FAILURE, &data);
	} else if (os_strncmp(custom, "ASSOCINFO(ReqIEs=", 17) == 0) {
		char *spos;
		int bytes;

		spos = custom + 17;

		bytes = strspn(spos, "0123456789abcdefABCDEF");
		if (!bytes || (bytes & 1))
			return;
		bytes /= 2;

		data.assoc_info.req_ies = os_malloc(bytes);
		if (data.assoc_info.req_ies == NULL)
			return;

		data.assoc_info.req_ies_len = bytes;
		hexstr2bin(spos, data.assoc_info.req_ies, bytes);

		spos += bytes * 2;

		data.assoc_info.resp_ies = NULL;
		data.assoc_info.resp_ies_len = 0;

		if (os_strncmp(spos, " RespIEs=", 9) == 0) {
			spos += 9;

			bytes = strspn(spos, "0123456789abcdefABCDEF");
			if (!bytes || (bytes & 1))
				goto done;
			bytes /= 2;

			data.assoc_info.resp_ies = os_malloc(bytes);
			if (data.assoc_info.resp_ies == NULL)
				goto done;

			data.assoc_info.resp_ies_len = bytes;
			hexstr2bin(spos, data.assoc_info.resp_ies, bytes);
		}

		wpa_supplicant_event(ctx, EVENT_ASSOCINFO, &data);

	done:
		os_free(data.assoc_info.resp_ies);
		os_free(data.assoc_info.req_ies);
#ifdef CONFIG_PEERKEY
	} else if (os_strncmp(custom, "STKSTART.request=", 17) == 0) {
		if (
#ifdef CONFIG_TDLS
			0 && 
#endif /* CONFIG_TDLS */
			hwaddr_aton(custom + 17, data.stkstart.peer)) {
			wpa_printf(MSG_DEBUG, "WEXT: unrecognized "
				   "STKSTART.request '%s'", custom + 17);
			return;
		}
#ifdef CONFIG_TDLS
		char * spos = custom+17;

		os_memcpy(data.stkstart.peer, spos, ETH_ALEN);
		spos += ETH_ALEN;
		data.stkstart.smk_type = *(u16*)spos;
		spos += 2;
		data.stkstart.smk_len = *(u16*)spos;
		spos += 2;
		data.stkstart.smk_data = spos;
#endif /* CONFIG_TDLS */
		wpa_supplicant_event(ctx, EVENT_STKSTART, &data);
#endif /* CONFIG_PEERKEY */
	}
}


static int wpa_driver_wext_event_wireless_michaelmicfailure(
	void *ctx, const char *ev, size_t len)
{
	const struct iw_michaelmicfailure *mic;
	union wpa_event_data data;

	if (len < sizeof(*mic))
		return -1;

	mic = (const struct iw_michaelmicfailure *) ev;

	wpa_printf(MSG_DEBUG, "Michael MIC failure wireless event: "
		   "flags=0x%x src_addr=" MACSTR, mic->flags,
		   MAC2STR(mic->src_addr.sa_data));

	os_memset(&data, 0, sizeof(data));
	data.michael_mic_failure.unicast = !(mic->flags & IW_MICFAILURE_GROUP);
	wpa_supplicant_event(ctx, EVENT_MICHAEL_MIC_FAILURE, &data);

	return 0;
}


static int wpa_driver_wext_event_wireless_pmkidcand(
	struct wpa_driver_wext_data *drv, const char *ev, size_t len)
{
	const struct iw_pmkid_cand *cand;
	union wpa_event_data data;
	const u8 *addr;

	if (len < sizeof(*cand))
		return -1;

	cand = (const struct iw_pmkid_cand *) ev;
	addr = (const u8 *) cand->bssid.sa_data;

	wpa_printf(MSG_DEBUG, "PMKID candidate wireless event: "
		   "flags=0x%x index=%d bssid=" MACSTR, cand->flags,
		   cand->index, MAC2STR(addr));

	os_memset(&data, 0, sizeof(data));
	os_memcpy(data.pmkid_candidate.bssid, addr, ETH_ALEN);
	data.pmkid_candidate.index = cand->index;
	data.pmkid_candidate.preauth = cand->flags & IW_PMKID_CAND_PREAUTH;
	wpa_supplicant_event(drv->ctx, EVENT_PMKID_CANDIDATE, &data);

	return 0;
}


#if 0   /* was, before HACK */
static int wpa_driver_wext_event_wireless_assocreqie(
	struct wpa_driver_wext_data *drv, const char *ev, int len)
{
	if (len < 0)
		return -1;

	wpa_hexdump(MSG_DEBUG, "AssocReq IE wireless event", (const u8 *) ev,
		    len);
	os_free(drv->assoc_req_ies);
	drv->assoc_req_ies = os_malloc(len);
	if (drv->assoc_req_ies == NULL) {
		drv->assoc_req_ies_len = 0;
		return -1;
	}
	os_memcpy(drv->assoc_req_ies, ev, len);
	drv->assoc_req_ies_len = len;

	return 0;
}
#endif


#if 0   /* was, before HACK */
static int wpa_driver_wext_event_wireless_assocrespie(
	struct wpa_driver_wext_data *drv, const char *ev, int len)
{
	if (len < 0)
		return -1;

	wpa_hexdump(MSG_DEBUG, "AssocResp IE wireless event", (const u8 *) ev,
		    len);
	os_free(drv->assoc_resp_ies);
	drv->assoc_resp_ies = os_malloc(len);
	if (drv->assoc_resp_ies == NULL) {
		drv->assoc_resp_ies_len = 0;
		return -1;
	}
	os_memcpy(drv->assoc_resp_ies, ev, len);
	drv->assoc_resp_ies_len = len;

	return 0;
}
#endif


static void wpa_driver_wext_event_assoc_ies(struct wpa_driver_wext_data *drv)
{
	union wpa_event_data data;

	if (drv->assoc_req_ies == NULL && drv->assoc_resp_ies == NULL)
		return;

	os_memset(&data, 0, sizeof(data));
	if (drv->assoc_req_ies) {
		data.assoc_info.req_ies = drv->assoc_req_ies;
		drv->assoc_req_ies = NULL;
		data.assoc_info.req_ies_len = drv->assoc_req_ies_len;
	}
	if (drv->assoc_resp_ies) {
		data.assoc_info.resp_ies = drv->assoc_resp_ies;
		drv->assoc_resp_ies = NULL;
		data.assoc_info.resp_ies_len = drv->assoc_resp_ies_len;
	}

	wpa_supplicant_event(drv->ctx, EVENT_ASSOCINFO, &data);

	os_free(data.assoc_info.req_ies);
	os_free(data.assoc_info.resp_ies);
}


static void wpa_driver_wext_event_wireless(struct wpa_driver_wext_data *drv,
					   void *ctx, char *data, int len)
{
	struct iw_event iwe_buf, *iwe = &iwe_buf;
	char *pos, *end, *custom, *buf;

	pos = data;
	end = data + len;

	while (pos + IW_EV_LCP_LEN <= end) {
		/* Event data may be unaligned, so make a local, aligned copy
		 * before processing. */
		os_memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
		wpa_printf(MSG_DEBUG, "Wireless event: cmd=0x%x len=%d",
			   iwe->cmd, iwe->len);
		if (iwe->len <= IW_EV_LCP_LEN)
			return;

		custom = pos + IW_EV_POINT_LEN;
		if (drv->we_version_compiled > 18 &&
		    (iwe->cmd == IWEVMICHAELMICFAILURE ||
		     iwe->cmd == IWEVCUSTOM ||
		     iwe->cmd == IWEVASSOCREQIE ||
		     iwe->cmd == IWEVASSOCRESPIE ||
		     iwe->cmd == IWEVPMKIDCAND)) {
			/* WE-19 removed the pointer from struct iw_point */
			char *dpos = (char *) &iwe_buf.u.data.length;
			int dlen = dpos - (char *) &iwe_buf;
			os_memcpy(dpos, pos + IW_EV_LCP_LEN,
				  sizeof(struct iw_event) - dlen);
		} else {
			os_memcpy(&iwe_buf, pos, sizeof(struct iw_event));
			custom += IW_EV_POINT_OFF;
		}

		switch (iwe->cmd) {
		case SIOCGIWAP:
			wpa_printf(MSG_DEBUG, "Wireless event: new AP: "
				   MACSTR,
				   MAC2STR((u8 *) iwe->u.ap_addr.sa_data));
			if (os_memcmp(iwe->u.ap_addr.sa_data,
				      "\x00\x00\x00\x00\x00\x00", ETH_ALEN) ==
			    0 ||
			    os_memcmp(iwe->u.ap_addr.sa_data,
				      "\x44\x44\x44\x44\x44\x44", ETH_ALEN) ==
			    0) {
				os_free(drv->assoc_req_ies);
				drv->assoc_req_ies = NULL;
				os_free(drv->assoc_resp_ies);
				drv->assoc_resp_ies = NULL;
				wpa_supplicant_event(ctx, EVENT_DISASSOC,
						     NULL);
			
			} else {
				wpa_driver_wext_event_assoc_ies(drv);
				wpa_supplicant_event(ctx, EVENT_ASSOC, NULL);
			}
			break;
		case IWEVMICHAELMICFAILURE:
			wpa_driver_wext_event_wireless_michaelmicfailure(
				ctx, custom, iwe->u.data.length);
			break;
		case IWEVCUSTOM:
		case IWEVASSOCREQIE:    /* HACK in driver to get around IWEVCUSTOM size limit */
			if (custom + iwe->u.data.length > end)
				return;
			buf = os_malloc(iwe->u.data.length + 1);
			if (buf == NULL)
				return;
			os_memcpy(buf, custom, iwe->u.data.length);
			buf[iwe->u.data.length] = '\0';
			wpa_driver_wext_event_wireless_custom(ctx, buf);
			os_free(buf);
			break;
		case SIOCGIWSCAN:
			eloop_cancel_timeout(scan_timeout,
					     drv, ctx);
			wpa_supplicant_event(ctx, EVENT_SCAN_RESULTS, NULL);
			break;
                #if 0   /* was, before HACK */
		case IWEVASSOCREQIE:
			wpa_driver_wext_event_wireless_assocreqie(
				drv, custom, iwe->u.data.length);
			break;
		case IWEVASSOCRESPIE:
			wpa_driver_wext_event_wireless_assocrespie(
				drv, custom, iwe->u.data.length);
			break;
                #endif  /* ... HACK */
		case IWEVPMKIDCAND:
			wpa_driver_wext_event_wireless_pmkidcand(
				drv, custom, iwe->u.data.length);
			break;
		}

		pos += iwe->len;
	}
}


static void wpa_driver_wext_event_link(void *ctx, char *buf, size_t len,
				       int del)
{
	union wpa_event_data event;

	os_memset(&event, 0, sizeof(event));
	if (len > sizeof(event.interface_status.ifname))
		len = sizeof(event.interface_status.ifname) - 1;
	os_memcpy(event.interface_status.ifname, buf, len);
	event.interface_status.ievent = del ? EVENT_INTERFACE_REMOVED :
		EVENT_INTERFACE_ADDED;

	wpa_printf(MSG_DEBUG, "RTM_%sLINK, IFLA_IFNAME: Interface '%s' %s",
		   del ? "DEL" : "NEW",
		   event.interface_status.ifname,
		   del ? "removed" : "added");

	wpa_supplicant_event(ctx, EVENT_INTERFACE_STATUS, &event);
}


static void wpa_driver_wext_event_rtm_newlink(struct wpa_driver_wext_data *drv,
					      void *ctx, struct nlmsghdr *h,
					      size_t len)
{
	struct ifinfomsg *ifi;
	int attrlen, nlmsg_len, rta_len;
	struct rtattr * attr;

	if (len < sizeof(*ifi))
		return;

	ifi = NLMSG_DATA(h);

	if (drv->ifindex != ifi->ifi_index && drv->ifindex2 != ifi->ifi_index)
	{
                #if 0   /* happens too much */
		wpa_printf(MSG_DEBUG, "Ignore event for foreign ifindex %d",
			   ifi->ifi_index);
                #endif
		return;
	}

	wpa_printf(MSG_DEBUG, "RTM_NEWLINK: operstate=%d ifi_flags=0x%x "
		   "(%s%s%s%s)",
		   drv->operstate, ifi->ifi_flags,
		   (ifi->ifi_flags & IFF_UP) ? "[UP]" : "",
		   (ifi->ifi_flags & IFF_RUNNING) ? "[RUNNING]" : "",
		   (ifi->ifi_flags & IFF_LOWER_UP) ? "[LOWER_UP]" : "",
		   (ifi->ifi_flags & IFF_DORMANT) ? "[DORMANT" : "");
	/*
	 * Some drivers send the association event before the operup event--in
	 * this case, lifting operstate in wpa_driver_wext_set_operstate()
	 * fails. This will hit us when wpa_supplicant does not need to do
	 * IEEE 802.1X authentication
	 */
	if (drv->operstate == 1 &&
	    (ifi->ifi_flags & (IFF_LOWER_UP | IFF_DORMANT)) == IFF_LOWER_UP &&
	    !(ifi->ifi_flags & IFF_RUNNING))
		wpa_driver_wext_send_oper_ifla(drv, -1, IF_OPER_UP);

	nlmsg_len = NLMSG_ALIGN(sizeof(struct ifinfomsg));

	attrlen = h->nlmsg_len - nlmsg_len;
	if (attrlen < 0)
		return;

	attr = (struct rtattr *) (((char *) ifi) + nlmsg_len);

	rta_len = RTA_ALIGN(sizeof(struct rtattr));
	while (RTA_OK(attr, attrlen)) {
		if (attr->rta_type == IFLA_WIRELESS) {
			wpa_driver_wext_event_wireless(
				drv, ctx, ((char *) attr) + rta_len,
				attr->rta_len - rta_len);
		} else if (attr->rta_type == IFLA_IFNAME) {
			wpa_driver_wext_event_link(ctx,
						   ((char *) attr) + rta_len,
						   attr->rta_len - rta_len, 0);
		}
		attr = RTA_NEXT(attr, attrlen);
	}
}


static void wpa_driver_wext_event_rtm_dellink(struct wpa_driver_wext_data *drv,
					      void *ctx, struct nlmsghdr *h,
					      size_t len)
{
	struct ifinfomsg *ifi;
	int attrlen, nlmsg_len, rta_len;
	struct rtattr * attr;

	if (len < sizeof(*ifi))
		return;

	ifi = NLMSG_DATA(h);

	nlmsg_len = NLMSG_ALIGN(sizeof(struct ifinfomsg));

	attrlen = h->nlmsg_len - nlmsg_len;
	if (attrlen < 0)
		return;

	attr = (struct rtattr *) (((char *) ifi) + nlmsg_len);

	rta_len = RTA_ALIGN(sizeof(struct rtattr));
	while (RTA_OK(attr, attrlen)) {
		if (attr->rta_type == IFLA_IFNAME) {
			wpa_driver_wext_event_link(ctx,
						   ((char *) attr) + rta_len,
						   attr->rta_len - rta_len, 1);
		}
		attr = RTA_NEXT(attr, attrlen);
	}
}


static void wpa_driver_wext_event_receive(int sock, void *eloop_ctx,
					  void *sock_ctx)
{
	char buf[8192];
	int left;
	struct sockaddr_nl from;
	socklen_t fromlen;
	struct nlmsghdr *h;
	int max_events = 10;

try_again:
	fromlen = sizeof(from);
	left = recvfrom(sock, buf, sizeof(buf), MSG_DONTWAIT,
			(struct sockaddr *) &from, &fromlen);
	if (left < 0) {
		if (errno != EINTR && errno != EAGAIN)
			perror("recvfrom(netlink)");
		return;
	}

	h = (struct nlmsghdr *) buf;
	while (left >= (int) sizeof(*h)) {
		int len, plen;

		len = h->nlmsg_len;
		plen = len - sizeof(*h);
		if (len > left || plen < 0) {
			wpa_printf(MSG_DEBUG, "Malformed netlink message: "
				   "len=%d left=%d plen=%d",
				   len, left, plen);
			break;
		}

		switch (h->nlmsg_type) {
		case RTM_NEWLINK:
			wpa_driver_wext_event_rtm_newlink(eloop_ctx, sock_ctx,
							  h, plen);
			break;
		case RTM_DELLINK:
			wpa_driver_wext_event_rtm_dellink(eloop_ctx, sock_ctx,
							  h, plen);
			break;
		}

		len = NLMSG_ALIGN(len);
		left -= len;
		h = (struct nlmsghdr *) ((char *) h + len);
	}

	if (left > 0) {
		wpa_printf(MSG_DEBUG, "%d extra bytes in the end of netlink "
			   "message", left);
	}

	if (--max_events > 0) {
		/*
		 * Try to receive all events in one eloop call in order to
		 * limit race condition on cases where AssocInfo event, Assoc
		 * event, and EAPOL frames are received more or less at the
		 * same time. We want to process the event messages first
		 * before starting EAPOL processing.
		 */
		goto try_again;
	}
}


static int wpa_driver_wext_get_ifflags_ifname(struct wpa_driver_wext_data *drv,
					      const char *ifname, int *flags)
{
	struct ifreq ifr;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(drv->ioctl_sock, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
		perror("ioctl[SIOCGIFFLAGS]");
		return -1;
	}
	*flags = ifr.ifr_flags & 0xffff;
	return 0;
}


/**
 * wpa_driver_wext_get_ifflags - Get interface flags (SIOCGIFFLAGS)
 * @drv: driver_wext private data
 * @flags: Pointer to returned flags value
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_get_ifflags(struct wpa_driver_wext_data *drv, int *flags)
{
	return wpa_driver_wext_get_ifflags_ifname(drv, drv->ifname, flags);
}


static int wpa_driver_wext_set_ifflags_ifname(struct wpa_driver_wext_data *drv,
					      const char *ifname, int flags)
{
	struct ifreq ifr;

	os_memset(&ifr, 0, sizeof(ifr));
	os_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_flags = flags & 0xffff;
	if (ioctl(drv->ioctl_sock, SIOCSIFFLAGS, (caddr_t) &ifr) < 0) {
		perror("SIOCSIFFLAGS");
		return -1;
	}
	return 0;
}


/**
 * wpa_driver_wext_set_ifflags - Set interface flags (SIOCSIFFLAGS)
 * @drv: driver_wext private data
 * @flags: New value for flags
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_ifflags(struct wpa_driver_wext_data *drv, int flags)
{
	return wpa_driver_wext_set_ifflags_ifname(drv, drv->ifname, flags);
}


/**
 * wpa_driver_wext_init - Initialize WE driver interface
 * @ctx: context to be used when calling wpa_supplicant functions,
 * e.g., wpa_supplicant_event()
 * @ifname: interface name, e.g., wlan0
 * Returns: Pointer to private data, %NULL on failure
 */
void * wpa_driver_wext_init(void *ctx, const char *ifname)
{
	int s, flags;
	struct sockaddr_nl local;
	struct wpa_driver_wext_data *drv;

	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;
	drv->ctx = ctx;
	os_strncpy(drv->ifname, ifname, sizeof(drv->ifname));

	drv->ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (drv->ioctl_sock < 0) {
		perror("socket(PF_INET,SOCK_DGRAM)");
		os_free(drv);
		return NULL;
	}

	s = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (s < 0) {
		perror("socket(PF_NETLINK,SOCK_RAW,NETLINK_ROUTE)");
		close(drv->ioctl_sock);
		os_free(drv);
		return NULL;
	}

	os_memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = RTMGRP_LINK;
	if (bind(s, (struct sockaddr *) &local, sizeof(local)) < 0) {
		perror("bind(netlink)");
		close(s);
		close(drv->ioctl_sock);
		os_free(drv);
		return NULL;
	}

	eloop_register_read_sock(s, wpa_driver_wext_event_receive, drv, ctx);
	drv->event_sock = s;

	drv->mlme_sock = -1;

	/*
	 * Make sure that the driver does not have any obsolete PMKID entries.
	 */
	wpa_driver_wext_flush_pmkid(drv);

	if (wpa_driver_wext_set_mode(drv, 0) < 0) {
		printf("Could not configure driver to use managed mode\n");
	}

	if (wpa_driver_wext_get_ifflags(drv, &flags) != 0 ||
	    wpa_driver_wext_set_ifflags(drv, flags | IFF_UP) != 0) {
		printf("Could not set interface '%s' UP\n", drv->ifname);
	}

	wpa_driver_wext_get_range(drv);

	drv->ifindex = if_nametoindex(drv->ifname);

	if (os_strncmp(ifname, "wlan", 4) == 0) {
		/*
		 * Host AP driver may use both wlan# and wifi# interface in
		 * wireless events. Since some of the versions included WE-18
		 * support, let's add the alternative ifindex also from
		 * driver_wext.c for the time being. This may be removed at
		 * some point once it is believed that old versions of the
		 * driver are not in use anymore.
		 */
		char ifname2[IFNAMSIZ + 1];
		os_strncpy(ifname2, ifname, sizeof(ifname2));
		os_memcpy(ifname2, "wifi", 4);
		wpa_driver_wext_alternative_ifindex(drv, ifname2);
	}

	wpa_driver_wext_send_oper_ifla(drv, 1, IF_OPER_DORMANT);

    // RAY: WAR since driver doesn't support Wireless EXT query
    drv->we_version_compiled = 19;
	return drv;
}


/**
 * wpa_driver_wext_deinit - Deinitialize WE driver interface
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 *
 * Shut down driver interface and processing of driver events. Free
 * private data buffer if one was allocated in wpa_driver_wext_init().
 */
void wpa_driver_wext_deinit(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	int flags;

	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);

	/*
	 * Clear possibly configured driver parameters in order to make it
	 * easier to use the driver after wpa_supplicant has been terminated.
	 */
	wpa_driver_wext_set_bssid(drv, (u8 *) "\x00\x00\x00\x00\x00\x00");

	wpa_driver_wext_send_oper_ifla(priv, 0, IF_OPER_UP);

	eloop_unregister_read_sock(drv->event_sock);
	if (drv->mlme_sock >= 0)
		eloop_unregister_read_sock(drv->mlme_sock);

#if 0 /* keep the interface up even deinit wpa_supplicant */
	if (wpa_driver_wext_get_ifflags(drv, &flags) == 0)
		(void) wpa_driver_wext_set_ifflags(drv, flags & ~IFF_UP);
#endif

#ifdef CONFIG_CLIENT_MLME
	if (drv->mlmedev[0] &&
	    wpa_driver_wext_get_ifflags_ifname(drv, drv->mlmedev, &flags) == 0)
		(void) wpa_driver_wext_set_ifflags_ifname(drv, drv->mlmedev,
							  flags & ~IFF_UP);
#endif /* CONFIG_CLIENT_MLME */

	close(drv->event_sock);
	close(drv->ioctl_sock);
	if (drv->mlme_sock >= 0)
		close(drv->mlme_sock);
	os_free(drv->assoc_req_ies);
	os_free(drv->assoc_resp_ies);
	os_free(drv);
}


/**
 * wpa_driver_wext_scan_timeout - Scan timeout to report scan completion
 * @eloop_ctx: Unused
 * @timeout_ctx: ctx argument given to wpa_driver_wext_init()
 *
 * This function can be used as registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
void wpa_driver_wext_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	wpa_printf(MSG_DEBUG, "Scan timeout - try to get results");
	wpa_supplicant_event(timeout_ctx, EVENT_SCAN_RESULTS, NULL);
}


/**
 * wpa_driver_wext_scan - Request the driver to initiate scan
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: Specific SSID to scan for (ProbeReq) or %NULL to scan for
 *	all SSIDs (either active scan with broadcast SSID or passive
 *	scan
 * @ssid_len: Length of the SSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_scan(void *priv, const u8 *ssid, size_t ssid_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;
	struct iw_scan_req req;

	if (ssid_len > IW_ESSID_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: too long SSID (%lu)",
			   __FUNCTION__, (unsigned long) ssid_len);
		return -1;
	}

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);

	if (ssid && ssid_len) {
		os_memset(&req, 0, sizeof(req));
		req.essid_len = ssid_len;
		req.bssid.sa_family = ARPHRD_ETHER;
		os_memset(req.bssid.sa_data, 0xff, ETH_ALEN);
		os_memcpy(req.essid, ssid, ssid_len);
		iwr.u.data.pointer = (caddr_t) &req;
		iwr.u.data.length = sizeof(req);
		iwr.u.data.flags = IW_SCAN_THIS_ESSID;
	}

	if (ioctl(drv->ioctl_sock, SIOCSIWSCAN, &iwr) < 0) {
		perror("ioctl[SIOCSIWSCAN]");
		ret = -1;
	}

	/* Not all drivers generate "scan completed" wireless event, so try to
	 * read results after a timeout. */
	eloop_register_timeout(3, 0, wpa_driver_wext_scan_timeout, drv,
			       drv->ctx);

	return ret;
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


/**
 * wpa_driver_wext_get_scan_results - Fetch the latest scan results
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @results: Pointer to buffer for scan results
 * @max_size: Maximum number of entries (buffer size)
 * Returns: Number of scan result entries used on success, -1 on
 * failure
 *
 * If scan results include more than max_size BSSes, max_size will be
 * returned and the remaining entries will not be included in the
 * buffer.
 */
int wpa_driver_wext_get_scan_results(void *priv,
				     struct wpa_scan_result *results,
				     size_t max_size)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	size_t ap_num = 0;
	int first, maxrate;
	u8 *res_buf;
	struct iw_event iwe_buf, *iwe = &iwe_buf;
	char *pos, *end, *custom, *genie, *gpos, *gend;
	struct iw_param p;
	size_t len, clen, res_buf_len;

	os_memset(results, 0, max_size * sizeof(struct wpa_scan_result));

	res_buf_len = IW_SCAN_MAX_DATA;
	for (;;) {
		res_buf = os_malloc(res_buf_len);
		if (res_buf == NULL)
			return -1;
		os_memset(&iwr, 0, sizeof(iwr));
		os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
		iwr.u.data.pointer = res_buf;
		iwr.u.data.length = res_buf_len;

		if (ioctl(drv->ioctl_sock, SIOCGIWSCAN, &iwr) == 0)
			break;

		if (errno == E2BIG && res_buf_len < 100000) {
			os_free(res_buf);
			res_buf = NULL;
			res_buf_len *= 2;
			wpa_printf(MSG_DEBUG, "Scan results did not fit - "
				   "trying larger buffer (%lu bytes)",
				   (unsigned long) res_buf_len);
		} else {
			perror("ioctl[SIOCGIWSCAN]");
			os_free(res_buf);
			return -1;
		}
	}

	len = iwr.u.data.length;
	ap_num = 0;
	first = 1;

	pos = (char *) res_buf;
	end = (char *) res_buf + len;

	while (pos + IW_EV_LCP_LEN <= end) {
		int ssid_len;
		/* Event data may be unaligned, so make a local, aligned copy
		 * before processing. */
		os_memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
		if (iwe->len <= IW_EV_LCP_LEN)
			break;

		custom = pos + IW_EV_POINT_LEN;
		if (drv->we_version_compiled > 18 &&
		    (iwe->cmd == SIOCGIWESSID ||
		     iwe->cmd == SIOCGIWENCODE ||
		     iwe->cmd == IWEVGENIE ||
		     iwe->cmd == IWEVASSOCREQIE ||
		     iwe->cmd == IWEVCUSTOM)) {
			/* WE-19 removed the pointer from struct iw_point */
			char *dpos = (char *) &iwe_buf.u.data.length;
			int dlen = dpos - (char *) &iwe_buf;
			os_memcpy(dpos, pos + IW_EV_LCP_LEN,
				  sizeof(struct iw_event) - dlen);
		} else {
			os_memcpy(&iwe_buf, pos, sizeof(struct iw_event));
			custom += IW_EV_POINT_OFF;
		}

		switch (iwe->cmd) {
		case SIOCGIWAP:
			if (!first)
				ap_num++;
			first = 0;
			if (ap_num < max_size) {
				os_memcpy(results[ap_num].bssid,
					  iwe->u.ap_addr.sa_data, ETH_ALEN);
			}
			break;
		case SIOCGIWMODE:
			if (ap_num >= max_size)
				break;
			if (iwe->u.mode == IW_MODE_ADHOC)
				results[ap_num].caps |= IEEE80211_CAP_IBSS;
			else if (iwe->u.mode == IW_MODE_MASTER ||
				 iwe->u.mode == IW_MODE_INFRA)
				results[ap_num].caps |= IEEE80211_CAP_ESS;
			break;
		case SIOCGIWESSID:
			ssid_len = iwe->u.essid.length;
			if (custom + ssid_len > end)
				break;
			if (iwe->u.essid.flags &&
			    ssid_len > 0 &&
			    ssid_len <= IW_ESSID_MAX_SIZE) {
				if (ap_num < max_size) {
					os_memcpy(results[ap_num].ssid, custom,
						  ssid_len);
					results[ap_num].ssid_len = ssid_len;
				}
			}
			break;
		case SIOCGIWFREQ:
			if (ap_num < max_size) {
#ifdef MODIFIED_BY_SONY
				if (iwe->u.freq.m > 1000) {
					/* freq structure means frequency */
#endif	/* MODIFIED_BY_SONY */
				int divi = 1000000, i;
				if (iwe->u.freq.e == 0) {
					/*
					 * Some drivers do not report
					 * frequency, but a channel. Try to map
					 * this to frequency by assuming they
					 * are using IEEE 802.11b/g.
					 */
					if (iwe->u.freq.m >= 1 &&
					    iwe->u.freq.m <= 13) {
						results[ap_num].freq =
							2407 +
							5 * iwe->u.freq.m;
						break;
					} else if (iwe->u.freq.m == 14) {
						results[ap_num].freq = 2484;
						break;
					}
				}
				if (iwe->u.freq.e > 6) {
					wpa_printf(
						MSG_DEBUG, "Invalid freq "
						"in scan results (BSSID="
						MACSTR ": m=%d e=%d\n",
						MAC2STR(results[ap_num].bssid),
						iwe->u.freq.m, iwe->u.freq.e);
					break;
				}
				for (i = 0; i < iwe->u.freq.e; i++)
					divi /= 10;
				results[ap_num].freq = iwe->u.freq.m / divi;
#ifdef MODIFIED_BY_SONY
				} else {
					/* freq structure means channel */
					u32 freq = 0, channel = iwe->u.freq.m;
					if (channel < 34) {
						freq = 2412 + 5 * (channel - 1);
					} else if (channel > 0) {
						freq = 5170 + 5 * (channel - 34);
					}
					results[ap_num].freq = freq;
				}
#endif	/* MODIFIED_BY_SONY */
			}
			break;
		case IWEVQUAL:
			if (ap_num < max_size) {
#ifndef MODIFIED_BY_SONY
				results[ap_num].qual = iwe->u.qual.qual;
				results[ap_num].noise = iwe->u.qual.noise;
				results[ap_num].level = iwe->u.qual.level;
#else	/* MODIFIED_BY_SONY */
				results[ap_num].qual = (s8)iwe->u.qual.qual;
				results[ap_num].noise = (s8)iwe->u.qual.noise;
				results[ap_num].level = (s8)iwe->u.qual.level;
#endif	/* MODIFIED_BY_SONY */
			}
			break;
		case SIOCGIWENCODE:
			if (ap_num < max_size &&
			    !(iwe->u.data.flags & IW_ENCODE_DISABLED))
				results[ap_num].caps |= IEEE80211_CAP_PRIVACY;
			break;
		case SIOCGIWRATE:
			custom = pos + IW_EV_LCP_LEN;
			clen = iwe->len;
			if (custom + clen > end)
				break;
			maxrate = 0;
			while (((ssize_t) clen) >=
			       (ssize_t) sizeof(struct iw_param)) {
				/* Note: may be misaligned, make a local,
				 * aligned copy */
				os_memcpy(&p, custom, sizeof(struct iw_param));
				if (p.value > maxrate)
					maxrate = p.value;
				clen -= sizeof(struct iw_param);
				custom += sizeof(struct iw_param);
			}
			if (ap_num < max_size)
				results[ap_num].maxrate = maxrate;
			break;
		case IWEVGENIE:
			if (ap_num >= max_size)
				break;
			gpos = genie = custom;
			gend = genie + iwe->u.data.length;
			if (gend > end) {
				wpa_printf(MSG_INFO, "IWEVGENIE overflow");
				break;
			}
			while (gpos + 1 < gend &&
			       gpos + 2 + (u8) gpos[1] <= gend) {
				u8 ie = gpos[0], ielen = gpos[1] + 2;
#ifndef EAP_WPS
				if (ielen > SSID_MAX_WPA_IE_LEN) {
					gpos += ielen;
					continue;
				}
#endif /* EAP_WPS */
				switch (ie) {
				case GENERIC_INFO_ELEM:
#ifndef EAP_WPS
					if (ielen < 2 + 4 ||
					    os_memcmp(&gpos[2],
						      "\x00\x50\xf2\x01", 4) !=
					    0)
						break;
#else /* EAP_WPS */
					if (ielen >= 2 + 4 &&
					    os_memcmp(&gpos[2],
						   "\x00\x50\xf2\x01", 4) == 0 &&
						   ielen <= SSID_MAX_WPA_IE_LEN) {
#endif /* EAP_WPS */
					os_memcpy(results[ap_num].wpa_ie, gpos,
						  ielen);
					results[ap_num].wpa_ie_len = ielen;
#ifdef EAP_WPS
					} else if (ielen >= 2 + 4 &&
					    os_memcmp(&gpos[2],
						   "\x00\x50\xf2\x04", 4) == 0 /* &&
						   ielen <= SSID_MAX_WPS_IE_LEN */) {
						os_memcpy(results[ap_num].wps_ie, gpos,
						       ielen);
						results[ap_num].wps_ie_len = ielen;
					}
#endif /* EAP_WPS */
					break;
				case RSN_INFO_ELEM:
#ifdef EAP_WPS
					if (ielen <= SSID_MAX_WPA_IE_LEN) {
#endif /* EAP_WPS */
					os_memcpy(results[ap_num].rsn_ie, gpos,
						  ielen);
					results[ap_num].rsn_ie_len = ielen;
#ifdef EAP_WPS
					}
#endif /* EAP_WPS */
					break;
				}
				gpos += ielen;
			}
			break;
		case IWEVCUSTOM:
		case IWEVASSOCREQIE:    /* HACK in driver to get around IWEVCUSTOM size limit */
			clen = iwe->u.data.length;
			if (custom + clen > end)
				break;
			if (clen > 7 &&
			    os_strncmp(custom, "wpa_ie=", 7) == 0 &&
			    ap_num < max_size) {
				char *spos;
				int bytes;
				spos = custom + 7;
				bytes = custom + clen - spos;
				if (bytes & 1)
					break;
				bytes /= 2;
				if (bytes > SSID_MAX_WPA_IE_LEN) {
					wpa_printf(MSG_INFO, "Too long WPA IE "
						   "(%d)", bytes);
					break;
				}
				hexstr2bin(spos, results[ap_num].wpa_ie,
					   bytes);
				results[ap_num].wpa_ie_len = bytes;
			} else if (clen > 7 &&
				   os_strncmp(custom, "rsn_ie=", 7) == 0 &&
				   ap_num < max_size) {
				char *spos;
				int bytes;
				spos = custom + 7;
				bytes = custom + clen - spos;
				if (bytes & 1)
					break;
				bytes /= 2;
				if (bytes > SSID_MAX_WPA_IE_LEN) {
					wpa_printf(MSG_INFO, "Too long RSN IE "
						   "(%d)", bytes);
					break;
				}
				hexstr2bin(spos, results[ap_num].rsn_ie,
					   bytes);
				results[ap_num].rsn_ie_len = bytes;
			}
#ifdef EAP_WPS
			else if (clen > 7 &&
				   os_strncmp(custom, "wps_ie=", 7) == 0 &&
				   ap_num < max_size) {
				char *spos;
				int bytes;
				spos = custom + 7;
				bytes = custom + clen - spos;
				if (bytes & 1)
					break;
				bytes /= 2;
				if (bytes > SSID_MAX_WPS_IE_LEN) {
					wpa_printf(MSG_INFO, "Too long WPS IE "
						   "(%d)", bytes);
					break;
				}
				hexstr2bin(spos, results[ap_num].wps_ie,
					   bytes);
				results[ap_num].wps_ie_len = bytes;
			}
#endif /* EAP_WPS */
			break;
		}

		pos += iwe->len;
	}
	os_free(res_buf);
	res_buf = NULL;
	if (!first)
		ap_num++;
	if (ap_num > max_size) {
		wpa_printf(MSG_DEBUG, "Too small scan result buffer - "
			   "%lu BSSes but room only for %lu",
			   (unsigned long) ap_num,
			   (unsigned long) max_size);
		ap_num = max_size;
	}
	qsort(results, ap_num, sizeof(struct wpa_scan_result),
	      wpa_scan_result_compar);

	wpa_printf(MSG_DEBUG, "Received %lu bytes of scan results (%lu BSSes)",
		   (unsigned long) len, (unsigned long) ap_num);

	return ap_num;
}


static int wpa_driver_wext_get_range(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iw_range *range;
	struct iwreq iwr;
	int minlen;
	size_t buflen;

	/*
	 * Use larger buffer than struct iw_range in order to allow the
	 * structure to grow in the future.
	 */
	buflen = sizeof(struct iw_range) + 500;
	range = os_zalloc(buflen);
	if (range == NULL)
		return -1;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) range;
	iwr.u.data.length = buflen;

	minlen = ((char *) &range->enc_capa) - (char *) range +
		sizeof(range->enc_capa);

	if (ioctl(drv->ioctl_sock, SIOCGIWRANGE, &iwr) < 0) {
		perror("ioctl[SIOCGIWRANGE]");
		os_free(range);
		return -1;
	} else if (iwr.u.data.length >= minlen &&
		   range->we_version_compiled >= 18) {
		wpa_printf(MSG_DEBUG, "SIOCGIWRANGE: WE(compiled)=%d "
			   "WE(source)=%d enc_capa=0x%x",
			   range->we_version_compiled,
			   range->we_version_source,
			   range->enc_capa);
		drv->has_capability = 1;
		drv->we_version_compiled = range->we_version_compiled;
		if (range->enc_capa & IW_ENC_CAPA_WPA) {
			drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA |
				WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK;
		}
		if (range->enc_capa & IW_ENC_CAPA_WPA2) {
			drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
				WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK;
		}
		drv->capa.enc |= WPA_DRIVER_CAPA_ENC_WEP40 |
			WPA_DRIVER_CAPA_ENC_WEP104;
		if (range->enc_capa & IW_ENC_CAPA_CIPHER_TKIP)
			drv->capa.enc |= WPA_DRIVER_CAPA_ENC_TKIP;
		if (range->enc_capa & IW_ENC_CAPA_CIPHER_CCMP)
			drv->capa.enc |= WPA_DRIVER_CAPA_ENC_CCMP;
		wpa_printf(MSG_DEBUG, "  capabilities: key_mgmt 0x%x enc 0x%x",
			   drv->capa.key_mgmt, drv->capa.enc);
	} else {
		wpa_printf(MSG_DEBUG, "SIOCGIWRANGE: too old (short) data - "
			   "assuming WPA is not supported");
	}

	os_free(range);
	return 0;
}


static int wpa_driver_wext_set_wpa(void *priv, int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	return wpa_driver_wext_set_auth_param(drv, IW_AUTH_WPA_ENABLED,
					      enabled);
}


#if 0
#ifdef CONFIG_DRIVER_ADF
static int
set80211priv_adf_setkey(struct wpa_driver_wext_data *drv, int op, void *data, int len)
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

    if (adf_do_ioctl(drv->ioctl_sock,SIOCSADFWIOCTL, &i_req) < 0)
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

   	if (adf_do_ioctl(drv->ioctl_sock,SIOCSADFWIOCTL, &i_req) < 0) {
		int err = errno;
		perror("set80211priv_adf_setkey ioctl failed");
		wpa_printf(MSG_ERROR, "ioctl 0x%x failed errno=%d", op, err);
   		return EIO;
	}

	return 0;
}
#endif
#endif


#ifndef MODIFIED_BY_SONY
static int wpa_driver_wext_set_key_ext(void *priv, wpa_alg alg,
				       const u8 *addr, int key_idx,
				       int set_tx, const u8 *seq,
				       size_t seq_len,
				       const u8 *key, size_t key_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;
	struct iw_encode_ext *ext;

	if (seq_len > IW_ENCODE_SEQ_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: Invalid seq_len %lu",
			   __FUNCTION__, (unsigned long) seq_len);
		return -1;
	}

	ext = os_zalloc(sizeof(*ext) + key_len);
	if (ext == NULL)
		return -1;
	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.encoding.flags = key_idx + 1;
	if (alg == WPA_ALG_NONE)
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;
	iwr.u.encoding.pointer = (caddr_t) ext;
	iwr.u.encoding.length = sizeof(*ext) + key_len;

	if (addr == NULL ||
	    os_memcmp(addr, "\xff\xff\xff\xff\xff\xff", ETH_ALEN) == 0)
		ext->ext_flags |= IW_ENCODE_EXT_GROUP_KEY;
	if (set_tx)
		ext->ext_flags |= IW_ENCODE_EXT_SET_TX_KEY;

	ext->addr.sa_family = ARPHRD_ETHER;
	if (addr)
		os_memcpy(ext->addr.sa_data, addr, ETH_ALEN);
	else
		os_memset(ext->addr.sa_data, 0xff, ETH_ALEN);
	if (key && key_len) {
		os_memcpy(ext + 1, key, key_len);
		ext->key_len = key_len;
	}
	switch (alg) {
	case WPA_ALG_NONE:
		ext->alg = IW_ENCODE_ALG_NONE;
		break;
	case WPA_ALG_WEP:
		ext->alg = IW_ENCODE_ALG_WEP;
		break;
	case WPA_ALG_TKIP:
		ext->alg = IW_ENCODE_ALG_TKIP;
		break;
	case WPA_ALG_CCMP:
		ext->alg = IW_ENCODE_ALG_CCMP;
		break;
	default:
		wpa_printf(MSG_DEBUG, "%s: Unknown algorithm %d",
			   __FUNCTION__, alg);
		os_free(ext);
		return -1;
	}

	if (seq && seq_len) {
		ext->ext_flags |= IW_ENCODE_EXT_RX_SEQ_VALID;
		os_memcpy(ext->rx_seq, seq, seq_len);
	}

	if (ioctl(drv->ioctl_sock, SIOCSIWENCODEEXT, &iwr) < 0) {
		ret = errno == EOPNOTSUPP ? -2 : -1;
		if (errno == ENODEV) {
			/*
			 * ndiswrapper seems to be returning incorrect error
			 * code.. */
			ret = -2;
		}

		perror("ioctl[SIOCSIWENCODEEXT]");
	}

	os_free(ext);
	return ret;
}
#endif /* MODIFIED_BY_SONY */


/**
 * wpa_driver_wext_set_key - Configure encryption key
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @priv: Private driver interface data
 * @alg: Encryption algorithm (%WPA_ALG_NONE, %WPA_ALG_WEP,
 *	%WPA_ALG_TKIP, %WPA_ALG_CCMP); %WPA_ALG_NONE clears the key.
 * @addr: Address of the peer STA or ff:ff:ff:ff:ff:ff for
 *	broadcast/default keys
 * @key_idx: key index (0..3), usually 0 for unicast keys
 * @set_tx: Configure this key as the default Tx key (only used when
 *	driver does not support separate unicast/individual key
 * @seq: Sequence number/packet number, seq_len octets, the next
 *	packet number to be used for in replay protection; configured
 *	for Rx keys (in most cases, this is only used with broadcast
 *	keys and set to zero for unicast keys)
 * @seq_len: Length of the seq, depends on the algorithm:
 *	TKIP: 6 octets, CCMP: 6 octets
 * @key: Key buffer; TKIP: 16-byte temporal key, 8-byte Tx Mic key,
 *	8-byte Rx Mic Key
 * @key_len: Length of the key buffer in octets (WEP: 5 or 13,
 *	TKIP: 32, CCMP: 16)
 * Returns: 0 on success, -1 on failure
 *
 * This function uses SIOCSIWENCODEEXT by default, but tries to use
 * SIOCSIWENCODE if the extended ioctl fails when configuring a WEP key.
 */
int wpa_driver_wext_set_key(void *priv, wpa_alg alg,
			    const u8 *addr, int key_idx,
			    int set_tx, const u8 *seq, size_t seq_len,
			    const u8 *key, size_t key_len)
{
	int i, ret = 0;
	struct wpa_driver_wext_data *drv = priv;

	athcfg_wcmd_t i_req;
	os_memset(&i_req,0, sizeof(athcfg_wcmd_t));

	athcfg_wcmd_keyinfo_t keyinfo;
	os_memset(&keyinfo, 0, sizeof(athcfg_wcmd_keyinfo_t));

	os_strncpy(i_req.if_name, drv->ifname, IFNAMSIZ);	

	keyinfo.ik_flags |= (key_idx + 1);

	if (alg == WPA_ALG_WEP) {
		keyinfo.ik_type = ATHCFG_WCMD_CIPHERMODE_WEP;
	}

	keyinfo.ik_keylen = key_len;
	for (i = 0; i < key_len; i++) {
		keyinfo.ik_keydata[i] = key[i];
	}

	i_req.type = ATHCFG_WCMD_SET_ENC;
	i_req.d_key = keyinfo;

	if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0)
		return EIO;

	if (set_tx == 1) {
		os_memset(&i_req,0, sizeof(athcfg_wcmd_t));
		os_memset(&keyinfo, 0, sizeof(athcfg_wcmd_keyinfo_t));
		os_strncpy(i_req.if_name, drv->ifname, IFNAMSIZ);
		keyinfo.ik_flags |= (key_idx + 1);
		i_req.type = ATHCFG_WCMD_SET_ENC;
		i_req.d_key = keyinfo;	

        	if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0)
                	return EIO;
	}

	return 0;

#if 0
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	wpa_printf(MSG_DEBUG, "%s: alg=%d key_idx=%d set_tx=%d seq_len=%lu "
		   "key_len=%lu",
		   __FUNCTION__, alg, key_idx, set_tx,
		   (unsigned long) seq_len, (unsigned long) key_len);

#ifndef MODIFIED_BY_SONY
	ret = wpa_driver_wext_set_key_ext(drv, alg, addr, key_idx, set_tx,
					  seq, seq_len, key, key_len);
	if (ret == 0)
		return 0;

	if (ret == -2 &&
	    (alg == WPA_ALG_NONE || alg == WPA_ALG_WEP)) {
		wpa_printf(MSG_DEBUG, "Driver did not support "
			   "SIOCSIWENCODEEXT, trying SIOCSIWENCODE");
		ret = 0;
	} else {
		wpa_printf(MSG_DEBUG, "Driver did not support "
			   "SIOCSIWENCODEEXT");
		return ret;
	}
#endif /* MODIFIED_BY_SONY */

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.encoding.flags = key_idx + 1;
	if (alg == WPA_ALG_NONE)
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;
	iwr.u.encoding.pointer = (caddr_t) key;
	iwr.u.encoding.length = key_len;

	if (ioctl(drv->ioctl_sock, SIOCSIWENCODE, &iwr) < 0) {
		perror("ioctl[SIOCSIWENCODE]");
		ret = -1;
	}

	if (set_tx && alg != WPA_ALG_NONE) {
		os_memset(&iwr, 0, sizeof(iwr));
		os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
		iwr.u.encoding.flags = key_idx + 1;
		iwr.u.encoding.pointer = (caddr_t) key;
		iwr.u.encoding.length = 0;
		if (ioctl(drv->ioctl_sock, SIOCSIWENCODE, &iwr) < 0) {
			perror("ioctl[SIOCSIWENCODE] (set_tx)");
			ret = -1;
		}
	}

	return ret;
#endif
}


static int wpa_driver_wext_set_countermeasures(void *priv,
					       int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	return wpa_driver_wext_set_auth_param(drv,
					      IW_AUTH_TKIP_COUNTERMEASURES,
					      enabled);
}


static int wpa_driver_wext_set_drop_unencrypted(void *priv,
						int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	drv->use_crypt = enabled;
	return wpa_driver_wext_set_auth_param(drv, IW_AUTH_DROP_UNENCRYPTED,
					      enabled);
}


#ifdef CONFIG_DRIVER_ADF
static int
set80211priv_adf_mlme(struct wpa_driver_wext_data *drv, int op, void *data, int len)
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

	if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0) {
		int err = errno;
		perror("set80211priv_adf_mlme ioctl failed");
		wpa_printf(MSG_ERROR, "ioctl 0x%x failed errno=%d", op, err);
   		return EIO;
	}

	return 0;
}
#endif

static int wpa_driver_wext_mlme(struct wpa_driver_wext_data *drv,
				const u8 *addr, int cmd, int reason_code)
{
#if 0
	struct iwreq iwr;
	struct iw_mlme mlme;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	os_memset(&mlme, 0, sizeof(mlme));
	mlme.cmd = cmd;
	mlme.reason_code = reason_code;
	mlme.addr.sa_family = ARPHRD_ETHER;
	os_memcpy(mlme.addr.sa_data, addr, ETH_ALEN);
	iwr.u.data.pointer = (caddr_t) &mlme;
	iwr.u.data.length = sizeof(mlme);

	if (ioctl(drv->ioctl_sock, SIOCSIWMLME, &iwr) < 0) {
		perror("ioctl[SIOCSIWMLME]");
		ret = -1;
	}
#endif
	athcfg_wcmd_mlme_t mlme;
	int ret = 0;

	os_memset(&mlme, 0, sizeof(mlme));
	mlme.op = cmd;
	mlme.reason = reason_code;
	memcpy(mlme.mac.addr, addr, ATHCFG_WCMD_ADDR_LEN);

	if (set80211priv_adf_mlme(drv, ATHCFG_WCMD_SET_MLME, 
			&mlme, sizeof(mlme)) < 0) {
		perror("ioctl[SIOCSIWMLME]");
		ret = -1;
	}

	return ret;
}


static int wpa_driver_wext_deauthenticate(void *priv, const u8 *addr,
					  int reason_code)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	return wpa_driver_wext_mlme(drv, addr, IW_MLME_DEAUTH, reason_code);
}


static int wpa_driver_wext_disassociate(void *priv, const u8 *addr,
					int reason_code)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	return wpa_driver_wext_mlme(drv, addr, IW_MLME_DISASSOC,
				    reason_code);
}


static int wpa_driver_wext_set_gen_ie(void *priv, const u8 *ie,
				      size_t ie_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
#if 0
	struct iwreq iwr;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) ie;
	iwr.u.data.length = ie_len;

	if (ioctl(drv->ioctl_sock, SIOCSIWGENIE, &iwr) < 0) {
		perror("ioctl[SIOCSIWGENIE]");
		ret = -1;
	}
#endif
    athcfg_wcmd_t i_req;  
    memset(&i_req,0, sizeof(athcfg_wcmd_t));

    athcfg_wcmd_optie_t optie; 
    memset(&optie, 0, sizeof(athcfg_wcmd_optie_t));

    os_memcpy(optie.data, ie, ie_len);
    optie.len = ie_len;

    strncpy(i_req.if_name, drv->ifname, strlen(drv->ifname));
    i_req.type = ATHCFG_WCMD_SET_OPT_IE;
    i_req.d_optie = optie;

    if (adf_do_ioctl(drv->ioctl_sock, SIOCSADFWIOCTL, &i_req) < 0)
        return EIO;


	return ret;
}


static int wpa_driver_wext_cipher2wext(int cipher)
{
	switch (cipher) {
	case CIPHER_NONE:
		return IW_AUTH_CIPHER_NONE;
	case CIPHER_WEP40:
		return IW_AUTH_CIPHER_WEP40;
	case CIPHER_TKIP:
		return IW_AUTH_CIPHER_TKIP;
	case CIPHER_CCMP:
		return IW_AUTH_CIPHER_CCMP;
	case CIPHER_WEP104:
		return IW_AUTH_CIPHER_WEP104;
	default:
		return 0;
	}
}


static int wpa_driver_wext_keymgmt2wext(int keymgmt)
{
	switch (keymgmt) {
	case KEY_MGMT_802_1X:
		return IW_AUTH_KEY_MGMT_802_1X;
	case KEY_MGMT_PSK:
		return IW_AUTH_KEY_MGMT_PSK;
	default:
		return 0;
	}
}


#ifndef MODIFIED_BY_SONY
static int
#else /* MODIFIED_BY_SONY */
int
#endif /* MODIFIED_BY_SONY */
wpa_driver_wext_auth_alg_fallback(struct wpa_driver_wext_data *drv,
				  struct wpa_driver_associate_params *params)
{
	struct iwreq iwr;
	int ret = 0;

	wpa_printf(MSG_DEBUG, "WEXT: Driver did not support "
		   "SIOCSIWAUTH for AUTH_ALG, trying SIOCSIWENCODE");

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	/* Just changing mode, not actual keys */
	iwr.u.encoding.flags = 0;
	iwr.u.encoding.pointer = (caddr_t) NULL;
	iwr.u.encoding.length = 0;

#ifdef MODIFIED_BY_SONY
	iwr.u.data.flags |= IW_ENCODE_NOKEY;
#endif /* MODIFIED_BY_SONY */

	/*
	 * Note: IW_ENCODE_{OPEN,RESTRICTED} can be interpreted to mean two
	 * different things. Here they are used to indicate Open System vs.
	 * Shared Key authentication algorithm. However, some drivers may use
	 * them to select between open/restricted WEP encrypted (open = allow
	 * both unencrypted and encrypted frames; restricted = only allow
	 * encrypted frames).
	 */

	if (!drv->use_crypt) {
#ifndef MODIFIED_BY_SONY
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;
#else /* MODIFIED_BY_SONY */
		iwr.u.data.flags |= IW_ENCODE_DISABLED;
#endif /* MODIFIED_BY_SONY */
	} else {
		if (params->auth_alg & AUTH_ALG_OPEN_SYSTEM)
#ifndef MODIFIED_BY_SONY
			iwr.u.encoding.flags |= IW_ENCODE_OPEN;
#else /* MODIFIED_BY_SONY */
			iwr.u.data.flags |= IW_ENCODE_OPEN;
#endif /* MODIFIED_BY_SONY */
		if (params->auth_alg & AUTH_ALG_SHARED_KEY)
#ifndef MODIFIED_BY_SONY
			iwr.u.encoding.flags |= IW_ENCODE_RESTRICTED;
#else /* MODIFIED_BY_SONY */
			iwr.u.data.flags |= IW_ENCODE_RESTRICTED;
#endif /* MODIFIED_BY_SONY */
	}

	if (ioctl(drv->ioctl_sock, SIOCSIWENCODE, &iwr) < 0) {
		perror("ioctl[SIOCSIWENCODE]");
		ret = -1;
	}

	return ret;
}


static int
wpa_driver_wext_associate(void *priv,
			  struct wpa_driver_associate_params *params)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	int allow_unencrypted_eapol;
	int value;

	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	/*
	 * If the driver did not support SIOCSIWAUTH, fallback to
	 * SIOCSIWENCODE here.
	 */
	if (drv->auth_alg_fallback &&
	    wpa_driver_wext_auth_alg_fallback(drv, params) < 0)
		ret = -1;

	if (!params->bssid &&
	    wpa_driver_wext_set_bssid(drv, NULL) < 0)
		ret = -1;

	if (wpa_driver_wext_set_mode(drv, params->mode) < 0)
		ret = -1;
	/* TODO: should consider getting wpa version and cipher/key_mgmt suites
	 * from configuration, not from here, where only the selected suite is
	 * available */
	if (wpa_driver_wext_set_gen_ie(drv, params->wpa_ie, params->wpa_ie_len)
	    < 0)
		ret = -1;
	if (params->wpa_ie == NULL || params->wpa_ie_len == 0)
		value = IW_AUTH_WPA_VERSION_DISABLED;
	else if (params->wpa_ie[0] == RSN_INFO_ELEM)
		value = IW_AUTH_WPA_VERSION_WPA2;
	else
		value = IW_AUTH_WPA_VERSION_WPA;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_WPA_VERSION, value) < 0)
		ret = -1;
	value = wpa_driver_wext_cipher2wext(params->pairwise_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_CIPHER_PAIRWISE, value) < 0)
		ret = -1;
	value = wpa_driver_wext_cipher2wext(params->group_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_CIPHER_GROUP, value) < 0)
		ret = -1;
	value = wpa_driver_wext_keymgmt2wext(params->key_mgmt_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_KEY_MGMT, value) < 0)
		ret = -1;
	value = params->key_mgmt_suite != KEY_MGMT_NONE ||
		params->pairwise_suite != CIPHER_NONE ||
		params->group_suite != CIPHER_NONE ||
		params->wpa_ie_len;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_PRIVACY_INVOKED, value) < 0)
		ret = -1;

	/* Allow unencrypted EAPOL messages even if pairwise keys are set when
	 * not using WPA. IEEE 802.1X specifies that these frames are not
	 * encrypted, but WPA encrypts them when pairwise keys are in use. */
	if (params->key_mgmt_suite == KEY_MGMT_802_1X ||
	    params->key_mgmt_suite == KEY_MGMT_PSK)
		allow_unencrypted_eapol = 0;
	else
		allow_unencrypted_eapol = 1;
	
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_RX_UNENCRYPTED_EAPOL,
					   allow_unencrypted_eapol) < 0)
		ret = -1;
	if (params->freq && wpa_driver_wext_set_freq(drv, params->freq) < 0)
		ret = -1;
	if (wpa_driver_wext_set_ssid(drv, params->ssid, params->ssid_len) < 0)
		ret = -1;
	if (params->bssid &&
	    wpa_driver_wext_set_bssid(drv, params->bssid) < 0)
		ret = -1;

	return ret;
}


static int wpa_driver_wext_set_auth_alg(void *priv, int auth_alg)
{
	struct wpa_driver_wext_data *drv = priv;
	int algs = 0, res;

	if (auth_alg & AUTH_ALG_OPEN_SYSTEM)
		algs |= IW_AUTH_ALG_OPEN_SYSTEM;
	if (auth_alg & AUTH_ALG_SHARED_KEY)
		algs |= IW_AUTH_ALG_SHARED_KEY;
	if (auth_alg & AUTH_ALG_LEAP)
		algs |= IW_AUTH_ALG_LEAP;
	if (algs == 0) {
		/* at least one algorithm should be set */
		algs = IW_AUTH_ALG_OPEN_SYSTEM;
	}

	res = wpa_driver_wext_set_auth_param(drv, IW_AUTH_80211_AUTH_ALG,
					     algs);
	drv->auth_alg_fallback = res == -2;
	return res;
}


/**
 * wpa_driver_wext_set_mode - Set wireless mode (infra/adhoc), SIOCSIWMODE
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @mode: 0 = infra/BSS (associate with an AP), 1 = adhoc/IBSS
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_mode(void *priv, int mode)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.mode = mode ? IW_MODE_ADHOC : IW_MODE_INFRA;

	if (ioctl(drv->ioctl_sock, SIOCSIWMODE, &iwr) < 0) {
		perror("ioctl[SIOCSIWMODE]");
		ret = -1;
	}

	return ret;
}


static int wpa_driver_wext_pmksa(struct wpa_driver_wext_data *drv,
				 u32 cmd, const u8 *bssid, const u8 *pmkid)
{
	struct iwreq iwr;
	struct iw_pmksa pmksa;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	os_memset(&pmksa, 0, sizeof(pmksa));
	pmksa.cmd = cmd;
	pmksa.bssid.sa_family = ARPHRD_ETHER;
	if (bssid)
		os_memcpy(pmksa.bssid.sa_data, bssid, ETH_ALEN);
	if (pmkid)
		os_memcpy(pmksa.pmkid, pmkid, IW_PMKID_LEN);
	iwr.u.data.pointer = (caddr_t) &pmksa;
	iwr.u.data.length = sizeof(pmksa);

	if (ioctl(drv->ioctl_sock, SIOCSIWPMKSA, &iwr) < 0) {
		if (errno != EOPNOTSUPP)
			perror("ioctl[SIOCSIWPMKSA]");
		ret = -1;
	}

	return ret;
}


static int wpa_driver_wext_add_pmkid(void *priv, const u8 *bssid,
				     const u8 *pmkid)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_ADD, bssid, pmkid);
}


static int wpa_driver_wext_remove_pmkid(void *priv, const u8 *bssid,
		 			const u8 *pmkid)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_REMOVE, bssid, pmkid);
}


static int wpa_driver_wext_flush_pmkid(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_FLUSH, NULL, NULL);
}


static int wpa_driver_wext_get_capa(void *priv, struct wpa_driver_capa *capa)
{
	struct wpa_driver_wext_data *drv = priv;
	if (!drv->has_capability)
		return -1;
	os_memcpy(capa, &drv->capa, sizeof(*capa));
	return 0;
}


int wpa_driver_wext_alternative_ifindex(struct wpa_driver_wext_data *drv,
					const char *ifname)
{
	if (ifname == NULL) {
		drv->ifindex2 = -1;
		return 0;
	}

	drv->ifindex2 = if_nametoindex(ifname);
	if (drv->ifindex2 <= 0)
		return -1;

	wpa_printf(MSG_DEBUG, "Added alternative ifindex %d (%s) for "
		   "wireless events", drv->ifindex2, ifname);

	return 0;
}


int wpa_driver_wext_set_operstate(void *priv, int state)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_printf(MSG_DEBUG, "%s: operstate %d->%d (%s)",
		   __func__, drv->operstate, state, state ? "UP" : "DORMANT");
	drv->operstate = state;
	return wpa_driver_wext_send_oper_ifla(
		drv, -1, state ? IF_OPER_UP : IF_OPER_DORMANT);
}


#ifdef CONFIG_CLIENT_MLME
static int hostapd_ioctl(struct wpa_driver_wext_data *drv,
			 struct prism2_hostapd_param *param, int len)
{
	struct iwreq iwr;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) param;
	iwr.u.data.length = len;

	if (ioctl(drv->ioctl_sock, PRISM2_IOCTL_HOSTAPD, &iwr) < 0) {
		perror("ioctl[PRISM2_IOCTL_HOSTAPD]");
		return -1;
	}

	return 0;
}


static struct wpa_hw_modes *
wpa_driver_wext_get_hw_feature_data(void *priv, u16 *num_modes, u16 *flags)
{
	struct wpa_driver_wext_data *drv = priv;
	struct prism2_hostapd_param *param;
	u8 *pos, *end;
	struct wpa_hw_modes *modes = NULL;
	int i;

	param = os_zalloc(PRISM2_HOSTAPD_MAX_BUF_SIZE);
	if (param == NULL)
		return NULL;
	param->cmd = PRISM2_HOSTAPD_GET_HW_FEATURES;

	if (hostapd_ioctl(drv, param, PRISM2_HOSTAPD_MAX_BUF_SIZE) < 0) {
		perror("ioctl[PRISM2_IOCTL_HOSTAPD]");
		goto out;
	}

	*num_modes = param->u.hw_features.num_modes;
	*flags = param->u.hw_features.flags;

	pos = param->u.hw_features.data;
	end = pos + PRISM2_HOSTAPD_MAX_BUF_SIZE -
		(param->u.hw_features.data - (u8 *) param);

	modes = os_zalloc(*num_modes * sizeof(struct wpa_hw_modes));
	if (modes == NULL)
		goto out;

	for (i = 0; i < *num_modes; i++) {
		struct hostapd_ioctl_hw_modes_hdr *hdr;
		struct wpa_hw_modes *feature;
		int clen, rlen;

		hdr = (struct hostapd_ioctl_hw_modes_hdr *) pos;
		pos = (u8 *) (hdr + 1);
		clen = hdr->num_channels * sizeof(struct wpa_channel_data);
		rlen = hdr->num_rates * sizeof(struct wpa_rate_data);

		feature = &modes[i];
		switch (hdr->mode) {
		case MODE_IEEE80211A:
			feature->mode = WPA_MODE_IEEE80211A;
			break;
		case MODE_IEEE80211B:
			feature->mode = WPA_MODE_IEEE80211B;
			break;
		case MODE_IEEE80211G:
			feature->mode = WPA_MODE_IEEE80211G;
			break;
		case MODE_ATHEROS_TURBO:
		case MODE_ATHEROS_TURBOG:
			wpa_printf(MSG_ERROR, "Skip unsupported hw_mode=%d in "
				   "get_hw_features data", hdr->mode);
			pos += clen + rlen;
			continue;
		default:
			wpa_printf(MSG_ERROR, "Unknown hw_mode=%d in "
				   "get_hw_features data", hdr->mode);
			ieee80211_sta_free_hw_features(modes, *num_modes);
			modes = NULL;
			break;
		}
		feature->num_channels = hdr->num_channels;
		feature->num_rates = hdr->num_rates;

		feature->channels = os_malloc(clen);
		feature->rates = os_malloc(rlen);
		if (!feature->channels || !feature->rates ||
		    pos + clen + rlen > end) {
			ieee80211_sta_free_hw_features(modes, *num_modes);
			modes = NULL;
			break;
		}

		os_memcpy(feature->channels, pos, clen);
		pos += clen;
		os_memcpy(feature->rates, pos, rlen);
		pos += rlen;
	}

out:
	os_free(param);
	return modes;
}


int wpa_driver_wext_set_channel(void *priv, wpa_hw_mode phymode, int chan,
				int freq)
{
	return wpa_driver_wext_set_freq(priv, freq);
}


static void wpa_driver_wext_mlme_read(int sock, void *eloop_ctx,
				      void *sock_ctx)
{
	struct wpa_driver_wext_data *drv = eloop_ctx;
	int len;
	unsigned char buf[3000];
	struct ieee80211_frame_info *fi;
	struct ieee80211_rx_status rx_status;

	len = recv(sock, buf, sizeof(buf), 0);
	if (len < 0) {
		perror("recv[MLME]");
		return;
	}

	if (len < (int) sizeof(struct ieee80211_frame_info)) {
		wpa_printf(MSG_DEBUG, "WEXT: Too short MLME frame (len=%d)",
			   len);
		return;
	}

	fi = (struct ieee80211_frame_info *) buf;
	if (ntohl(fi->version) != IEEE80211_FI_VERSION) {
		wpa_printf(MSG_DEBUG, "WEXT: Invalid MLME frame info version "
			   "0x%x", ntohl(fi->version));
		return;
	}

	os_memset(&rx_status, 0, sizeof(rx_status));
	rx_status.ssi = ntohl(fi->ssi_signal);
	rx_status.channel = ntohl(fi->channel);

	ieee80211_sta_rx(drv->ctx, buf + sizeof(struct ieee80211_frame_info),
			 len - sizeof(struct ieee80211_frame_info),
			 &rx_status);
}


static int wpa_driver_wext_open_mlme(struct wpa_driver_wext_data *drv)
{
	int flags, ifindex, s, *i;
	struct sockaddr_ll addr;
	struct iwreq iwr;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	i = (int *) iwr.u.name;
	*i++ = PRISM2_PARAM_USER_SPACE_MLME;
	*i++ = 1;

	if (ioctl(drv->ioctl_sock, PRISM2_IOCTL_PRISM2_PARAM, &iwr) < 0) {
		wpa_printf(MSG_ERROR, "WEXT: Failed to configure driver to "
			   "use user space MLME");
		return -1;
	}

	ifindex = if_nametoindex(drv->mlmedev);
	if (ifindex == 0) {
		wpa_printf(MSG_ERROR, "WEXT: mlmedev='%s' not found",
			   drv->mlmedev);
		return -1;
	}

	if (wpa_driver_wext_get_ifflags_ifname(drv, drv->mlmedev, &flags) != 0
	    || wpa_driver_wext_set_ifflags_ifname(drv, drv->mlmedev,
						  flags | IFF_UP) != 0) {
		wpa_printf(MSG_ERROR, "WEXT: Could not set interface "
			   "'%s' UP", drv->mlmedev);
		return -1;
	}

	s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (s < 0) {
		perror("socket[PF_PACKET,SOCK_RAW]");
		return -1;
	}

	os_memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = ifindex;

	if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind(MLME)");
		return -1;
	}

	if (eloop_register_read_sock(s, wpa_driver_wext_mlme_read, drv, NULL))
	{
		wpa_printf(MSG_ERROR, "WEXT: Could not register MLME read "
			   "socket");
		close(s);
		return -1;
	}

	return s;
}


static int wpa_driver_wext_send_mlme(void *priv, const u8 *data,
				     size_t data_len)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret;

	ret = send(drv->mlme_sock, data, data_len, 0);
	if (ret < 0) {
		perror("send[MLME]");
		return -1;
	}

	return 0;
}


static int wpa_driver_wext_mlme_add_sta(void *priv, const u8 *addr,
					const u8 *supp_rates,
					size_t supp_rates_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct prism2_hostapd_param param;
	size_t len;

	os_memset(&param, 0, sizeof(param));
	param.cmd = PRISM2_HOSTAPD_ADD_STA;
	os_memcpy(param.sta_addr, addr, ETH_ALEN);
	len = supp_rates_len;
	if (len > sizeof(param.u.add_sta.supp_rates))
		len = sizeof(param.u.add_sta.supp_rates);
	os_memcpy(param.u.add_sta.supp_rates, supp_rates, len);
	return hostapd_ioctl(drv, &param, sizeof(param));
}


static int wpa_driver_wext_mlme_remove_sta(void *priv, const u8 *addr)
{
	struct wpa_driver_wext_data *drv = priv;
	struct prism2_hostapd_param param;

	os_memset(&param, 0, sizeof(param));
	param.cmd = PRISM2_HOSTAPD_REMOVE_STA;
	os_memcpy(param.sta_addr, addr, ETH_ALEN);
	return hostapd_ioctl(drv, &param, sizeof(param));
}

#endif /* CONFIG_CLIENT_MLME */


static int wpa_driver_wext_set_param(void *priv, const char *param)
{
#ifdef CONFIG_CLIENT_MLME
	struct wpa_driver_wext_data *drv = priv;
	const char *pos, *pos2;
	size_t len;

	if (param == NULL)
		return 0;

	wpa_printf(MSG_DEBUG, "%s: param='%s'", __func__, param);

	pos = os_strstr(param, "mlmedev=");
	if (pos) {
		pos += 8;
		pos2 = os_strchr(pos, ' ');
		if (pos2)
			len = pos2 - pos;
		else
			len = os_strlen(pos);
		if (len + 1 > sizeof(drv->mlmedev))
			return -1;
		os_memcpy(drv->mlmedev, pos, len);
		drv->mlmedev[len] = '\0';
		wpa_printf(MSG_DEBUG, "WEXT: Using user space MLME with "
			   "mlmedev='%s'", drv->mlmedev);
		drv->capa.flags |= WPA_DRIVER_FLAGS_USER_SPACE_MLME;

		drv->mlme_sock = wpa_driver_wext_open_mlme(drv);
		if (drv->mlme_sock < 0)
			return -1;
	}
#endif /* CONFIG_CLIENT_MLME */

	return 0;
}


int wpa_driver_wext_get_version(struct wpa_driver_wext_data *drv)
{
	return drv->we_version_compiled;
}


const struct wpa_driver_ops wpa_driver_wext_ops = {
	.name = "wext",
	.desc = "Linux wireless extensions (generic)",
	.get_bssid = wpa_driver_wext_get_bssid,
	.get_ssid = wpa_driver_wext_get_ssid,
	.set_wpa = wpa_driver_wext_set_wpa,
	.set_key = wpa_driver_wext_set_key,
	.set_countermeasures = wpa_driver_wext_set_countermeasures,
	.set_drop_unencrypted = wpa_driver_wext_set_drop_unencrypted,
	.scan = wpa_driver_wext_scan,
	.get_scan_results = wpa_driver_wext_get_scan_results,
	.deauthenticate = wpa_driver_wext_deauthenticate,
	.disassociate = wpa_driver_wext_disassociate,
	.associate = wpa_driver_wext_associate,
	.set_auth_alg = wpa_driver_wext_set_auth_alg,
	.init = wpa_driver_wext_init,
	.deinit = wpa_driver_wext_deinit,
	.set_param = wpa_driver_wext_set_param,
	.add_pmkid = wpa_driver_wext_add_pmkid,
	.remove_pmkid = wpa_driver_wext_remove_pmkid,
	.flush_pmkid = wpa_driver_wext_flush_pmkid,
	.get_capa = wpa_driver_wext_get_capa,
	.set_operstate = wpa_driver_wext_set_operstate,
#ifdef CONFIG_CLIENT_MLME
	.get_hw_feature_data = wpa_driver_wext_get_hw_feature_data,
	.set_channel = wpa_driver_wext_set_channel,
	.set_ssid = wpa_driver_wext_set_ssid,
	.set_bssid = wpa_driver_wext_set_bssid,
	.send_mlme = wpa_driver_wext_send_mlme,
	.mlme_add_sta = wpa_driver_wext_mlme_add_sta,
	.mlme_remove_sta = wpa_driver_wext_mlme_remove_sta,
#endif /* CONFIG_CLIENT_MLME */
};
