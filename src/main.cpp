#include <Arduino.h>

/* SD SPI Pin config */
#define SD_CS_PIN           D8 // D4 - HandMade SD reader; D8 - D1-Mini

/* System led settings */
#define SYS_LED             LED_BUILTIN
#define LED_BL_ERROR_MS     100
#define LED_BL_START_MS     150
#define LED_BL_INIT_SD_MS   250
#define LED_BL_INIT_WM_MS   500
#define LED_BL_IN_AP_MODE   750
#define LED_BL_RUN_MS       1000

/* WiFi Manager options */
#define WM_AP_NAME      "ESP8266-" 
#define WM_AP_PASS      "12345678" 

/* FTP Server options */
#define FTP_LOGIN       "esp8266"
#define FTP_PASSW       "esp8266"

/* Debug serial options */
#define DEBUG_WM_ENABLE false

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266WebServer.h>
#include <ESP8266FtpServer.h>

#include <SPI.h>
#include <SD.h>

/*
 * D5, D6, D7, D8, 3V3 and G
 * The SD-Card Reader uses the SPI bus pins:
 * WEMOS    SD-CARD Reader
 * D5       <->     SCK/CLK
 * D6       <->     MISO
 * D7       <->     MOSI
 * D8 (D4)  <->     CS 
 * Vcc      <->     Vcc
 * Gnd      <->     Gnd
 * */

#include <Ticker.h>

Ticker ledTiker;
void ledBlink();

#define SYS_LED_BLINK(ms)     ledTiker.detach(); ledTiker.attach_ms(ms, ledBlink)

void ledInit(uint8_t pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void ledBlink()
{
  digitalWrite(SYS_LED, !digitalRead(SYS_LED));
}

const char* ftpUser = FTP_LOGIN;
const char* ftpPass = FTP_PASSW;

//WiFiManager
WiFiManager wifiManager;
const String wmApName = WM_AP_NAME + String(ESP.getChipId(), HEX);
const String wmApPassw = WM_AP_PASS;

void onApModeCallback(WiFiManager *wm)
{
  Serial.println("WiFi SSID not found!!!");
  Serial.printf("Switch to AP Mode SSID: %s \r\n", wmApName.c_str());
  SYS_LED_BLINK(LED_BL_IN_AP_MODE);
}

FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

void setup() 
{
  ledInit(SYS_LED);

  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println();

  Serial.printf("----===== ESP8266 FTP SERVER (ID: %08X) =====---- \r\n", ESP.getChipId());
  // Serial.printf("WM_AP: %s, WM_PSW: %s \r\n", wmApName.c_str(), wmApPassw.c_str());

  Serial.println("\nInitializing SD card...");
  SYS_LED_BLINK(LED_BL_START_MS);

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!SD.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    SYS_LED_BLINK(LED_BL_INIT_SD_MS);
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
    SYS_LED_BLINK(LED_BL_ERROR_MS);
  }

  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (SD.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }
  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(SD.fatType(), DEC);

  volumesize = SD.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= SD.totalClusters();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)

  Serial.printf("Volume size:       %.2f Gb (%d Mb) \r\n", 
    (float)((volumesize/1024) / 1024.0),
    volumesize/1024);

  Serial.println("Startup WiFi Connection...");
  wifiManager.setDebugOutput(DEBUG_WM_ENABLE);
  wifiManager.setAPCallback(onApModeCallback);

  wifiManager.autoConnect(wmApName.c_str(), wmApPassw.c_str());
  Serial.println("WiFi connected OK");
  Serial.printf("Connected to SSID: %s \r\n", WiFi.SSID().c_str());
  Serial.printf("IP address: %s \r\n", WiFi.localIP().toString().c_str());

  SYS_LED_BLINK(LED_BL_INIT_WM_MS);

  Serial.print("\nStartup FTP server...");
  ftpSrv.begin(ftpUser, ftpPass); 
  Serial.println("OK");  

  SYS_LED_BLINK(LED_BL_RUN_MS);
}

void loop() 
{
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!! 
}
