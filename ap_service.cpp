#include "ap_service.h"

extern bool wifiIsRunning = false;

WifiConfig loadWifiConfig(SdFat *sdWifi)
{
    WifiConfig cfg;
    
    File file = (*sdWifi).open("config/wifi.env", FILE_READ);
    if (!file)
    {
        Serial.println("wifi.env introuvable, valeurs par défaut utilisées");
        cfg.ssid = "RTRIP-GPS";
        cfg.password = "rtrip1234";
        return cfg;
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.isEmpty() || line.startsWith("#"))
            continue;

        int sep = line.indexOf('=');
        if (sep < 0)
            continue;

        String key = line.substring(0, sep);
        String value = line.substring(sep + 1);

        key.trim();
        value.trim();

        if (key == "WIFI_SSID")
            cfg.ssid = value;
        else if (key == "WIFI_PASSWORD")
            cfg.password = value;
        else if (key == "WIFI_CHANNEL")
            cfg.channel = value.toInt();
        else if (key == "WIFI_HIDDEN")
            cfg.hidden = (value == "true");
    }

    file.close();
    cfg.success = true;
    return cfg;
}

void setupWifi(WifiConfig *cfg){
  if(!(*cfg).success){
    #ifdef DEBUG_MODE
    Serial.println("Cannot setup the wifi AP! AP Configuration is may not initiated rightly");
    #endif
    wifiIsRunning = false;
 false;
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP((*cfg).ssid, (*cfg).password);
  wifiIsRunning = true;

  (*cfg).ip = WiFi.softAPIP().toString();
}

void stopWifi(){
  if(!wifiIsRunning) return;

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiIsRunning = false;
}
