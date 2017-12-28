/*
Based on code provided by Robert Oostenveld
  https://github.com/robertoostenveld/arduino/tree/master/esp8266_artnet_neopixel

This version is using E1.31 re sACN i/o ArtNet.
In addition the LED stripes are APA102 using the SPI interface.

SACNview can be used to test the module.

   This sketch receive a DMX universes via E1.31 to control a
   strip of APA102 leds via Adafruit's DotStar library:

   https://github.com/rstephan/ArtnetWifi
   https://github.com/adafruit/Adafruit_NeoPixel
 */

#include <ESP8266WiFi.h>                // https://github.com/esp8266/Arduino
#include <E131.h>                       //  https://github.com/forkineye/E131
#define UNIVERSE 1                      /* Universe to listen for */
#define CHANNEL_START 1                 /* Channel to start listening at */
E131 e131;                              // E131 instance

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// #include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
// #include <WiFiClient.h>
// #include <ArtnetWifi.h>          // https://github.com/rstephan/ArtnetWifi
// #include <Adafruit_NeoPixel.h>   // https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_DotStar.h>   // https://github.com/adafruit/Adafruit_DotStar
#include <SPI.h>         // COMMENT OUT THIS LINE FOR GEMMA OR TRINKET
#include <FS.h>

#include "setup_ota.h"
#include "neopixel_mode.h"

#include "global.h"

Config config;
ESP8266WebServer server(80);
const char* host = "ARTNET";
const char* version = __DATE__ " / " __TIME__;
float temperature = 0, fps = 0;

// Neopixel settings
//const byte dataPin = D2;
//Adafruit_NeoPixel strip_org = Adafruit_NeoPixel(1, dataPin, NEO_GRBW + NEO_KHZ800); // start with one pixel

#define NUMPIXELS 144 // Number of LEDs in strip
#define CLOCKPIN    SCK   // D5 - GPIO14  HSCLK - SPI bus with ID 1 = HSPI
#define DATAPIN     MOSI  // D7 - GPIO13 HCS    - SPI bus with ID 1 = HSPI
// Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

uint32_t debug_timeout = millis();
uint32_t debug2_timeout = millis();
#define DEBUG_TIMEOUT 1000

// Artnet settings
// ArtnetWifi artnet;
unsigned int packetCounter = 0;

// Global universe buffer
struct {
        uint16_t universe;
        uint16_t length;
        uint8_t sequence;
        uint8_t *data;
} global;

// use an array of function pointers to jump to the desired mode
void (*mode[])(uint16_t, uint16_t, uint8_t, uint8_t *) {
        mode0, mode1, mode2, mode3, mode4, mode5, mode6, mode7, mode8, mode9, mode10, mode11, mode12, mode13, mode14, mode15, mode16
};

// keep the timing of the function calls
long tic_loop = 0, tic_fps = 0, tic_packet = 0, tic_web = 0;
long frameCounter = 0;

#define WIFI_CONNECT_TIMEOUT 10000
// ------------------------------------------------------------------------------------- WiFiConnect
// Wifi Connection
void WifiConnect() {

  DEBUGGING("--------------------------------------------------- WifiConnect ");
	DEBUGGING_L(">> SSID: ");
	DEBUGGING(WIFI_SSID);

  /* Choose one to begin listening for E1.31 data */
  e131.begin(WIFI_SSID, WIFI_PASS);                       /* via Unicast on the default port */
  //e131.beginMulticast(ssid, passphrase, UNIVERSE);  /* via Multicast for Universe 1 */


} // WifiConnect

// ------------------------------------------------------------------------------------- WiFiConnect
// start Access Point
void WifiConnectAP() {

	DEBUGGING("--------------------------------------------------- WifiConnectAP ");
	DEBUGGING_L(">> SSID: ");
	DEBUGGING(ACCESS_POINT_NAME);

	// AP mode
	// WiFi.mode(WIFI_AP); // Access point (AP) mode, where it creates its own network that others can join
  	WiFi.mode(WIFI_AP_STA);
  	WiFi.softAP(ACCESS_POINT_NAME , ACCESS_POINT_PASSWORD);
  	IPAddress myIP = WiFi.softAPIP();

  	DEBUGGING_L(">> WiFi - connected to Access Point with IP ");
  	DEBUGGING(myIP);
  	// Led_Blink(pLED_RED,100, 300);
  	// Led_Blink(pLED_RED,100, 300);
  	SoftAPup = true;

} //WifiConnectAP

//this will be called for each UDP packet received
void onDmxPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t * data) {
        // print some feedback
        Serial.print("onDmxPacket> packetCounter = ");
        Serial.print(packetCounter++);

        Serial.print("  //  universe : ");
        Serial.print(universe);

        if ((millis() - tic_fps) > 1000 && frameCounter > 100) {
                // don't estimate the FPS too quickly
                fps = 1000 * frameCounter / (millis() - tic_fps);
                tic_fps = millis();
                frameCounter = 0;
                Serial.print(",  FPS = ");
                Serial.print(fps);
        }
        Serial.println();

        // copy the data from the UDP packet over to the global universe buffer
        global.universe = universe;
        global.sequence = sequence;
        if (length < 512)
                global.length = length;
        for (int i = 0; i < global.length; i++)
                global.data[i] = data[i];
} // onDmxpacket

void updateNeopixelStrip(void) {
        // update the neopixel strip configuration
        strip.updateLength(config.pixels);
        strip.setBrightness(config.brightness);
        /*
        if (config.leds == 3)
                strip.updateType(NEO_GRB + NEO_KHZ800);
        else if (config.leds == 4 && config.white)
                strip.updateType(NEO_GRBW + NEO_KHZ800);
        else if (config.leds == 4 && !config.white)
                strip.updateType(NEO_GRBW + NEO_KHZ800);
        */
}

void setup() {
        Serial.begin(115200);
        while (!Serial) {
                ;
        }
        Serial.println("setup starting");

        global.universe = 1;
        global.sequence = 0;
        global.length = 512;
        global.data = (uint8_t *)malloc(512);

        SPIFFS.begin();
        strip.begin();

        Serial.println("fullBlack");
        fullBlack();
        Serial.println("setPixelColor");
        strip.setPixelColor(0, strip.Color(100, 0, 0 ) );
        Serial.println("show");
        strip.show();
        delay(500);
        strip.setPixelColor(0, strip.Color(0, 100, 0 ) );
        strip.show();
        delay(500);
        strip.setPixelColor(0, strip.Color(0, 0, 100 ) );
        strip.show();
        delay(500);
        strip.setPixelColor(0, strip.Color(100, 100, 100 ) );
        strip.show();

        initialConfig();

        // saveConfig();

        if (loadConfig()) {
                Serial.println("1 - updateNeopixelStrip");
                updateNeopixelStrip();
                Serial.println("setBrightness");
                strip.setBrightness(255);
                Serial.println("singleYellow");
                singleYellow();
                delay(1000);
        }
        else {
                Serial.println("2 - updateNeopixelStrip");
                updateNeopixelStrip();
                Serial.println("setBrightness");
                strip.setBrightness(255);
                Serial.println("singleRed");
                singleRed();
                delay(1000);
        }

        Serial.println("WifiConnectAP");
        WifiConnect();
        // WiFiManager wifiManager;
        // wifiManager.resetSettings();
        //wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
        //wifiManager.autoConnect(host);
        //Serial.println("connected");
        //Serial.println("WiFi.status");
        //if (WiFi.status() == WL_CONNECTED)
        //        singleGreen();

        // this serves all URIs that can be resolved to a file on the SPIFFS filesystem
        Serial.println("server.onNotFound");
        server.onNotFound(handleNotFound);

        server.on("/", HTTP_GET, []() {
                tic_web = millis();
                handleRedirect("/index");
        });

        server.on("/index", HTTP_GET, []() {
                tic_web = millis();
                handleStaticFile("/index.html");
        });

        server.on("/defaults", HTTP_GET, []() {
                tic_web = millis();
                Serial.println("handleDefaults");
                handleStaticFile("/reload_success.html");
                delay(2000);
                singleRed();
                initialConfig();
                saveConfig();
                //WiFiManager wifiManager;
                //wifiManager.resetSettings();
                ESP.restart();
        });

        server.on("/reconnect", HTTP_GET, []() {
                tic_web = millis();
                Serial.println("handleReconnect");
                handleStaticFile("/reload_success.html");
                delay(2000);
                singleYellow();
                //WiFiManager wifiManager;
                //wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
                //wifiManager.startConfigPortal(host);
                Serial.println("connected");
                if (WiFi.status() == WL_CONNECTED)
                        singleGreen();
        });

        server.on("/reset", HTTP_GET, []() {
                tic_web = millis();
                Serial.println("handleReset");
                handleStaticFile("/reload_success.html");
                delay(2000);
                singleRed();
                ESP.restart();
        });

        server.on("/monitor", HTTP_GET, [] {
                tic_web = millis();
                handleStaticFile("/monitor.html");
        });

        server.on("/hello", HTTP_GET, [] {
                tic_web = millis();
                handleStaticFile("/hello.html");
        });

        server.on("/settings", HTTP_GET, [] {
                tic_web = millis();
                handleStaticFile("/settings.html");
        });

        server.on("/dir", HTTP_GET, [] {
                tic_web = millis();
                handleDirList();
        });

        server.on("/json", HTTP_PUT, [] {
                tic_web = millis();
                handleJSON();
        });

        server.on("/json", HTTP_POST, [] {
                tic_web = millis();
                handleJSON();
        });

        server.on("/json", HTTP_GET, [] {
                tic_web = millis();
                StaticJsonBuffer<300> jsonBuffer;
                JsonObject& root = jsonBuffer.createObject();
                CONFIG_TO_JSON(universe, "universe");
                CONFIG_TO_JSON(offset, "offset");
                CONFIG_TO_JSON(pixels, "pixels");
                CONFIG_TO_JSON(leds, "leds");
                CONFIG_TO_JSON(white, "white");
                CONFIG_TO_JSON(brightness, "brightness");
                CONFIG_TO_JSON(hsv, "hsv");
                CONFIG_TO_JSON(mode, "mode");
                CONFIG_TO_JSON(reverse, "reverse");
                CONFIG_TO_JSON(speed, "speed");
                CONFIG_TO_JSON(position, "position");
                root["version"] = version;
                root["uptime"]  = long(millis() / 1000);
                root["packets"] = packetCounter;
                root["fps"]     = fps;
                String str;
                root.printTo(str);
                server.send(200, "application/json", str);
        });

        server.on("/update", HTTP_GET, [] {
                tic_web = millis();
                handleStaticFile("/update.html");
        });

        server.on("/update", HTTP_POST, handleUpdate1, handleUpdate2);

        // start the web server
        server.begin();

        // announce the hostname and web server through zeroconf
        MDNS.begin(host);
        MDNS.addService("http", "tcp", 80);

        // artnet.begin();
        // artnet.setArtDmxCallback(onDmxPacket);

        // initialize all timers
        tic_loop   = millis();
        tic_packet = millis();
        tic_fps    = millis();
        tic_web    = 0;

        debug_timeout = millis();
        debug2_timeout = millis();
        Serial.println("setup done");
} // setup

void myDebug(String strTopic) {
  static int i=0;

  if (millis() - debug_timeout > DEBUG_TIMEOUT ) {
    Serial.print("DEBUG> ");
    Serial.print(strTopic);
    Serial.print(" ");
    Serial.println(i++);

    Serial.print("universe :");
    Serial.println(global.universe);
    Serial.print("length   :");
    Serial.println(global.length);
    Serial.print("sequence :");
    Serial.println(global.sequence);
    Serial.print("data     :");
    Serial.print(global.data[0]);
    Serial.print(" - ");
    Serial.print(global.data[1]);
    Serial.print(" - ");
    Serial.println(global.data[2]);

    debug_timeout = millis();
    }
} // myDebug

void myDebug2(String strTopic) {
  static int i=0;
  float intensity;

  if (millis() - debug2_timeout > DEBUG_TIMEOUT ) {
    Serial.print("DEBUG2> ");
    Serial.print(strTopic);
    Serial.print(" ");
    Serial.println(i++);

    Serial.print("offset    :");
    Serial.println(config.offset);

    Serial.print("data     :");
    Serial.println(global.data[0]);

    intensity = 1. * global.data[0] / 255.;
    Serial.print("intensity     :");
    Serial.println(intensity);

    debug2_timeout = millis();
    }
} // myDebug

void loop() {
        server.handleClient();

        if (WiFi.status() != WL_CONNECTED) { // check if WiFi is conencted
                singleRed();
                // WifiConnect();
        }
        else if ((millis() - tic_web) < 5000) {
                singleBlue();
        }
        else  {
                // artnet.read();
                if(e131.parsePacket()) {
                  if (e131.universe == global.universe) {

                    for (int i = 0; i < NUMPIXELS; i++) {
                      global.data[i] = e131.data[i];
                      //int j = i * 3 + (CHANNEL_START - 1);
                      //strip.setPixelColor(i, e131.data[j], e131.data[j+1], e131.data[j+2]);
                    }
                  // strip.show();
                }
              }
                // myDebug("after artnet.read");

                // this section gets executed at a maximum rate of around 1Hz
                if ((millis() - tic_loop) > 999)
                        updateNeopixelStrip();

                // this section gets executed at a maximum rate of around 100Hz
                if ((millis() - tic_loop) > 9) {
                        if (config.mode >= 0 && config.mode < (sizeof(mode) / 4)) {
                                // call the function corresponding to the current mode
                                (*mode[config.mode])(global.universe, global.length, global.sequence, global.data);
                                tic_loop = millis();
                                frameCounter++;
                        }
                }
        }

        delay(1);
} // loop
