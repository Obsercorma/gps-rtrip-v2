#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>
#include <WebServer.h>
#include <SdFat.h>
#include <WiFi.h>


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

bool setupWifi(WifiConfig *cfg);

bool stopWifi();

#endif
