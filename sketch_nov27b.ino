#include "TinyGPSPlus.h"
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <DHT.h>
#include "ap_service.h"
#include "http_service.h"

//#define DEBUG_MODE
#define TEMP_OFFSET 0.0
#define TIME_INTERVAL 8000
#define TIME_INTERVAL_END_BTN 2000
#define TIME_INTERVAL_REACH_BTN 2000
#define TIME_INTERVAL_STEP_BTN 1000
#define TIME_FLUSH_INTERVAL 5000
#define COUNT_TO_TRIGGER_WEB 2

// ------------------- PINS ----------------------
#define SD_CS 5
#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23

#define RX_GPS_PIN 16
#define TX_GPS_PIN 17

#define DHT_PIN 15
#define DHT_TYPE DHT11

#define RED_PIN 13   // G13
#define GREEN_PIN 12 // G12
#define BLUE_PIN 14  // G14

#define BTN_STEP 26
#define BTN_DEST 27

// ------------------- OBJETS ----------------------
HardwareSerial gpsDevice(2);
TinyGPSPlus gps;
SdFat sd;
FsFile fileWrite;
DHT dht(DHT_PIN, DHT_TYPE);
WifiConfig wifiCfg;

// ------------------- TIMERS ----------------------
unsigned long tGPSPrint = 0;
unsigned long tSensorRead = 0;
unsigned long tBlink = 0;
unsigned long tSDflush = 0;

bool ledState = false;
bool blinkActive = false;
bool endBtnTriggered = false;
uint8_t blinkPin = BLUE_PIN;
uint8_t posWebTrigger = 0;
uint8_t blinkTarget = 0;
uint8_t blinkCount = 0;
uint16_t blinkInterval = 300;
uint32_t timeCBtn = 0;
uint32_t ct = 0;
bool stepPressed = false;
bool reachPressed = false;
bool blinkState = false;
bool blinkLong = false;
bool hasPaused = false;
bool hasFailed = false;
bool doneProcess = false;
bool apRunning = false;
bool fileTripAlreadyRenamed = false;
unsigned long debounceStep = 0;
unsigned long debounceReach = 0;
unsigned long blinkStart = 0;
String fileTripName = "trips/trip.csv";

//----------------------------------------------------

void fileTripOpen(FsFile *f){
  FsFile checkDir = sd.open("trips", O_RDONLY);
  if(!checkDir.isDirectory())
  {
    #ifdef DEBUG_MODE
    Serial.println("Folder trip missing. Creating a new one...");
    #endif
    sd.mkdir("trips");
    checkDir.close();
  }
  (*f) = sd.open(fileTripName, O_WRITE | O_CREAT | O_APPEND);

    if (!(*f) || !f)
  {
  #ifdef DEBUG_MODE
      Serial.println("File open failed!");
  #endif
    hasFailed = true;
  }
}

void startBlink(uint8_t pin, uint8_t count, uint16_t speed, bool longBlink = false)
{
  blinkPin = pin;
  blinkTarget = count;
  blinkInterval = speed;
  blinkActive = true;
  blinkState = false;
  blinkCount = 0;
  blinkLong = longBlink;
  blinkStart = millis();
  digitalWrite(pin, LOW);
}
void blinkAlertNonBlocking(uint8_t pin, uint16_t interval)
{
  blinkActive = true;
  blinkPin = pin;
  blinkInterval = interval;
}

bool readButton(uint8_t pin, bool &state, unsigned long &debounce)
{
  unsigned long now = millis();
  if (digitalRead(pin) == LOW)
  {
    if (now - debounce > 200)
    {
      debounce = now;
      state = true;
      return true;
    }
  }
  return false;
}
void setStatus_RED_ERROR()
{ // Red blink
  startBlink(RED_PIN, 6, 200);
}

void setStatus_LED_SOLID(uint16_t pin)
{ // GPS no data
  blinkActive = false;
  digitalWrite(pin, HIGH);
}

void setStatus_GREEN_OK()
{ // Permanent green
  blinkActive = false;
  digitalWrite(GREEN_PIN, HIGH);
}

void setStatus_GREEN_END()
{ // Green blink
  startBlink(GREEN_PIN, 3, 300);
}

void status_BLUE_invalidGPS()
{ // 2× blink
  startBlink(BLUE_PIN, 2, 200);
}

void status_BLUE_step()
{ // 4× blink
  startBlink(BLUE_PIN, 4, 200);
}

void status_BLUE_reach()
{ // 1× long blink
  startBlink(BLUE_PIN, 4, 1000);
}

void status_start_and_stop()
{
  setStatus_LED_SOLID(RED_PIN);
  delay(500);
  digitalWrite(RED_PIN, LOW);
  setStatus_LED_SOLID(GREEN_PIN);
  delay(500);
  digitalWrite(GREEN_PIN, LOW);
  setStatus_LED_SOLID(BLUE_PIN);
  delay(500);
  digitalWrite(BLUE_PIN, LOW);
}

void startWiFiAndWebMode(){
  wifiCfg = loadWifiConfig(&sd);
  if(wifiCfg.success){
    apRunning = setupWifi(&wifiCfg);
    http_start(sd);
    #ifdef DEBUG_MODE
    Serial.printf("AP Mode Triggered; AP_SSID: %s - IP: %s\n", wifiCfg.ssid, wifiCfg.ip);
    #endif
    digitalWrite(BLUE_PIN, LOW);
    digitalWrite(RED_PIN, LOW);
    setStatus_GREEN_OK();
  }
}

//----------------------------------------------------
void setup()
{
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(BTN_STEP, INPUT_PULLUP);
  pinMode(BTN_DEST, INPUT_PULLUP);

  dht.begin();

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!sd.begin(SD_CS, SD_SCK_MHZ(10)))
  {
#ifdef DEBUG_MODE
    Serial.println("SD init failed !");
#endif
    setStatus_RED_ERROR();
    hasFailed = true;
  }

  fileTripOpen(&fileWrite);

  gpsDevice.begin(9600, SERIAL_8N1, RX_GPS_PIN, TX_GPS_PIN);
  if (!hasFailed)
  {
#ifdef DEBUG_MODE
    Serial.println("GPS Started");
#endif
    status_start_and_stop();
  }
}

//----------------------------------------------------
void loop()
{
  http_loop();
  
  if (doneProcess)
  {
    if(!apRunning) startWiFiAndWebMode();
    delay(500);
    return;
  }
  bool vr = digitalRead(BTN_STEP);
  bool ve = digitalRead(BTN_DEST);
  delay(25);
  if (endBtnTriggered)
  {
    doneProcess = true;
#ifdef DEBUG_MODE
    Serial.println("Closing all interfaces..");
#endif
    status_start_and_stop();
  }
  if (ve == LOW)
  {
    timeCBtn = millis();
    do
    {
      ve = digitalRead(BTN_DEST);
      delay(10);
    } while (ve == LOW);
    ct = (millis() - timeCBtn);
    if (ct >= TIME_INTERVAL_END_BTN)
    {
      endBtnTriggered = true;
      fileWrite.println("RT_END");
      setStatus_GREEN_END();
    }else{
      posWebTrigger++;
      if(posWebTrigger == COUNT_TO_TRIGGER_WEB){
        posWebTrigger = 0;
        #ifdef DEBUG_MODE
        Serial.println("Enabling the web & ap service");
        #endif
        apRunning = !apRunning;
        if(apRunning){
          fileWrite.close();
          startWiFiAndWebMode();
        }else{
          fileTripOpen(&fileWrite);
          if(wifiCfg.success) {
            http_stop();
            stopWifi();
          }
          #ifdef DEBUG_MODE
          if(fileWrite) Serial.println("AP Mode Disabled. Going back to the recording mode.");
          #endif
        }
      }
    }
  }
  else if (vr == LOW)
  {
    timeCBtn = millis();
    do
    {
      vr = digitalRead(BTN_STEP);
      delay(10);
    } while (vr == LOW);
    ct = (millis() - timeCBtn);
    digitalWrite(GREEN_PIN, LOW);
    if (ct >= TIME_INTERVAL_REACH_BTN)
    {
      fileWrite.println("DEST_R");
#ifdef DEBUG_MODE
      Serial.println("DEST_R");
#endif
      status_BLUE_reach();
    }
    else
    {
      hasPaused = !hasPaused;
#ifdef DEBUG_MODE
      Serial.println(hasPaused ? "RT_PAUSED" : "RT_RESUMED");
#endif
      fileWrite.println(hasPaused ? "RT_P" : "RT_R");
      status_BLUE_step();
      delay(200);
    }
  }
  if(apRunning) {
    delay(5);
    return;
  }

  while (gpsDevice.available())
  {
    gps.encode(gpsDevice.read());
  }

  unsigned long now = millis();

  if (blinkActive && now - tBlink >= blinkInterval && blinkCount < blinkTarget)
  {
    tBlink = now;
    ledState = !ledState;
    blinkCount++;
    digitalWrite(blinkPin, ledState);
  }

  if (hasFailed)
  {
    setStatus_LED_SOLID(RED_PIN);
  }
  else if (now - tSensorRead >= TIME_INTERVAL)
  {
    tSensorRead = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    bool coordsOK = gps.location.isValid() && gps.location.isUpdated();
    bool dateOK = gps.date.isValid() && gps.date.isUpdated() && gps.date.month() != 0;
    bool timeOK = gps.time.isValid() && gps.time.isUpdated();

    if(!fileTripAlreadyRenamed && dateOK){
      fileTripAlreadyRenamed = true;
      char name[64];
      snprintf(
        name,
        sizeof(name),
        "trips/trip_%04d%02d%02d.csv",
        gps.date.year(), gps.date.month(), gps.date.day()
      );
      fileTripName = "";
      fileTripName.concat(name);
      fileWrite.close();
      fileTripOpen(&fileWrite);
      #ifdef DEBUG_MODE
      Serial.printf("Writing on %s\n", name);
      #endif
    }

    if (!coordsOK || !dateOK || !timeOK)
    {
      digitalWrite(GREEN_PIN, LOW);
      status_BLUE_reach();
    }
    else
    {
      digitalWrite(GREEN_PIN, HIGH);
      blinkActive = false;
      digitalWrite(BLUE_PIN, LOW);
    }

    String data = coordsOK ? (String(gps.location.lat(), 8) + ";" + String(gps.location.lng(), 8) + ";" + String(gps.altitude.meters())) : "LT_I;LG_I;AT_I";
    data += ";" + (dateOK ? (String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year()) + "#V") : "0/0/2000#NV");
    data += ";" + (timeOK ? (String(gps.time.hour()) + ":" + String(gps.time.minute()) + "#V") : "0:0#NV");

    data += ";" + (!isnan(h) && !isnan(t) ? (String)dht.computeHeatIndex((t - TEMP_OFFSET), h, false) + "#V" : "0.0#NV");

    fileWrite.println(data);

#ifdef DEBUG_MODE
    Serial.println(data);
#endif
  }

  if (now - tSDflush >= TIME_FLUSH_INTERVAL && !hasFailed)
  {
    tSDflush = now;
    fileWrite.flush();
  }
}
