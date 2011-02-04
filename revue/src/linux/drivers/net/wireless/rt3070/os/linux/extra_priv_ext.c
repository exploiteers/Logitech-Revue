/**
 * This file added by GoogleTV team.
 *
 * Implement extra private IOCTL operations needed by wpa_supplicant.
 * Android note: look at the interface routines in
 * android_net_wifi_Wifi.cpp, although this also probably fulfills the
 * wireless_ext spec.
 */

#include "rt_config.h"

extern int rt_ioctl_giwrate(struct net_device *dev,
                            struct iw_request_info *info,
                            union iwreq_data *wrqu, char *extra);

INT extra_priv_ext(IN struct net_device *dev,
                   IN struct iwreq * wrq)
{
  char * cmd = (char *) wrq->u.data.pointer;
  int len = wrq->u.data.length;
  INT result = 0;
  PRTMP_ADAPTER pAd;

  GET_PAD_FROM_NET_DEV(pAd, dev);

  if ((strcmp("LINKSPEED", cmd) == 0)) {
    char extra;
    union iwreq_data wrq_data;

    (void) rt_ioctl_giwrate(dev, NULL, &wrq_data, &extra);

    // ralink driver reports in bits-per-second, convert to MBps
    // note that this produces the value "5" for the 5.5MBps
    // speed supported by 802.11b.
    wrq->u.data.length = snprintf(cmd, len, "LinkSpeed %d\n",
        wrq_data.bitrate.value / 1000000);
  } else if ((strcmp("RSSI", cmd) == 0)) {
    // return result string: "RSSI <int-val>" where int-val is the
    // received signal strength, typically a negative number
    wrq->u.data.length = snprintf(cmd, len, "rssi %d\n",
              pAd->StaCfg.RssiSample.LastRssi0 - pAd->BbpRssiToDbmDelta);
  } else if ((strncmp("POWERMODE", cmd, 9) == 0)) {
    // implement command string "POWERMODE {1,0}" from
    // WifiStateTracker.java, mode 1 is "Active", mode 0 is "Auto".
    // assume that active mode is high-power transmissions, and auto
    // mode tries to optimize transmit power based upon recent
    // packet-transfer statistics.
    //
    // Ignore this request for now.
  } else if ((strcmp("STOP", cmd) == 0)) {
    // implement command string "STOP"
    // stop packet filtering
    result = -EOPNOTSUPP;
  } else if ((strcmp("MACADDR", cmd) == 0)) {
    UCHAR * mac = pAd->CurrentAddress;
    // return result string "Macaddr = XX:XX:XX:XX:XX:XX"
    wrq->u.data.length = snprintf(cmd, len,
                                  "Macaddr = %02X:%02X:%02X:%02X:%02X:%02X\n",
                                  mac[0], mac[1], mac[2],
                                  mac[3], mac[4], mac[5]);
    result = 0;
  } else if ((strcmp("RXFILTER-STOP", cmd) == 0)) {
    DBGPRINT(RT_DEBUG_ERROR, ("%s: trace: cmd is %s, len %d\n", __FUNCTION__,
                              cmd, (int) len));
    result = -EOPNOTSUPP;
  } else if ((strcmp("RXFILTER-ADD", cmd) == 0)) {
    DBGPRINT(RT_DEBUG_ERROR, ("%s: trace: cmd is %s, len %d\n", __FUNCTION__,
                              cmd, (int) len));
    result = -EOPNOTSUPP;
  } else if ((strcmp("BTCOEXSCAN-STOP", cmd) == 0)) {
    DBGPRINT(RT_DEBUG_ERROR, ("%s: trace: cmd is %s, len %d\n", __FUNCTION__,
                              cmd, (int) len));
    result = -EOPNOTSUPP;
  } else if ((strcmp("SCAN-PASSIVE", cmd) == 0)) {
    // SCAN-PASSIVE does not use 802.11 PROBE frames, in order to save power.
    // Not applicable on the GoogleTV, as we do not worry about battery usage.
    result = 0;
  } else if ((strcmp("SCAN-ACTIVE", cmd) == 0)) {
    // SCAN-ACTIVE uses the 802.11 PROBE frame to actively seek access points.
    // This is the mode that is always used by the RALink driver.$
    result = 0;
  } else {
    result = -EOPNOTSUPP;
    DBGPRINT(RT_DEBUG_ERROR, ("%s: trace: cmd is %s, len %d\n", __FUNCTION__,
                              cmd, (int) len));
  }

  return result;
}
