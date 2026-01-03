#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include "SdFat.h"


struct WifiConfig
{
  String ssid;
  String password;
  uint8_t channel = 1;
  bool hidden = false;
  bool success = false;
  String ip;
};

WifiConfig loadWifiConfig(SdFat *sdWifi);

void setupWifi(WifiConfig *cfg);

void stopWifi();

#endif
