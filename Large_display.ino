/*
   Noiasca Neopixel Display
   64 large display 64 bit - with lot of pixels per segment

   There was a user request, for a display with 7 pixels per segment,
   which will give 35 pixels per digit.
   obviously, 35 pixels don't fit in a 32bit bitmap, therefore you you must
   use a unsigned 64bit variable for the segsize_t

   64bit variables come with several restrictions. For example you will not be able to
   print such a large variable to the serial, because it's not implemented in the print.h class.

   Theoretically 9pixels x 7segments = 63 would fit into a 64bit Variable, but be careful with
   any bitshift you will need in your sketch.

   Warning: There a two default makros:

   _BV is part of Libc. The intended use is to create bitmasks for manipulating register values.
   #define _BV(bit) (1 << (bit))

   bit() is part of the Arduino implementation:
   #define bit(b) (1UL << (b))

   so non of them is good for 64bit. Therefore you must define our own makro if needed (see the code below)

   http://werner.rothschopf.net/202005_arduino_neopixel_display.htm

   by noiasca
   2021-05-44 4958/332
*/
/***************** Date / Time ******************/
const int FW_VERSION = 128;  // add reset - correction time hiver 1.25 mqtt to VMM
String name = "Large_display";
#include "D:\Arduino\Private.h"
long up_date = 0;
const int interval = 10;         // Get data on this interval 120 sec.
const long dataPostDelay = 180;  // interval update 2h.
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#include <ctype.h>  // test numeric
long rssi;
int go;
/* Initialise the WiFi and MQTT Client objects */
WiFiClient wifiClient;
#include "time.h"
#include <NTPtimeESP.h>
String time_str;
int sensorValue;
const byte LDR = 39;
const byte ledPin = 4;          // Which pin on the Arduino is connected to the NeoPixels?
const byte numDigits = 4;       // How many digits (numbers) are available on your display
const byte pixelPerDigit = 39;  // all pixels, including decimal point pixels if available at each digit
const byte startPixel = 0;      // start with this pixel on the strip
const byte addPixels = 0;       //0        // unregular additional pixels to be added to the strip (e.g. a double point for a clock 12:34)
#define RED 0x00FF0000
#define GREEN 0x0000FF00
#define BLUE 0x000000FF
#define YELLOW 0x00FFFF00
#define PURPLE 0x00FF00FF
#define WWHITE 0x00FFFFFF  // warm white
#define CWHITE 0xFF000000  // cold white

/*
   Segments are named and orded like this

           SEG_A
         S       S
         E       E
         G       G
         F       B
           SEG_G
         S       S
         E       E
         G       G
         E       C
           SEG_D

  in the following constants you have to define
  which pixel number belongs to which segment
*/
typedef uint64_t segsize_t;  // define the type of segsize_t to 64bit

#define B64(b) (1ULL << (b))  // remark: don't use the _BV(n) nor bit() makro for 64 bit variables

const segsize_t segment[8] {
  B64(0) | B64(1) | B64(2) | B64(3) | B64(4),       // SEG_A
  B64(5) | B64(6) | B64(7) | B64(8) | B64(9),       // SEG_B
  B64(10) | B64(11) | B64(12) | B64(13) | B64(14),  // SEG_C
  B64(15) | B64(16) | B64(17) | B64(18) | B64(19),  // SEG_D
  B64(20) | B64(21) | B64(22) | B64(23) | B64(24),  // SEG_E
  B64(25) | B64(26) | B64(27) | B64(28) | B64(29),  // SEG_F
  B64(30) | B64(31) | B64(32) | B64(33) | B64(34),  // SEG_G
  B64(35) | B64(36) | B64(37) | B64(38),            // SEG_DP
};

/* yes you can use the bit notation but it might becomes VERY LONG if you need 64 bits

  const segsize_t segment[8] {
  0b00000000000000000000000000000011111,  // SEG_A
  0b00000000000000000000000001111100000,  // SEG_B
  0b00000000000000000000111110000000000,  // SEG_C
  0b00000000000000011111000000000000000,  // SEG_D
  0b00000000001111100000000000000000000,  // SEG_E
  0b00000111110000000000000000000000000,  // SEG_F
  0b11111000000000000000000000000000000,  // SEG_G
  0                                       // SEG_DP   if you don't have a decimal point, just leave it zero
  };
*/
const uint16_t ledCount(pixelPerDigit * numDigits + addPixels);
int offsetLogic_cb(uint16_t position)  // your callback function to keep track of additional pixels
{
  uint16_t offset = 0;
  if (position > 1) offset = addPixels;  // example: the additional Pixels are between 2nd and 3rd digit
  return offset;                         // you MUST return a value. It can be 0
}

#include <Adafruit_NeoPixel.h>                                     // install Adafruit library from library manager
Adafruit_NeoPixel strip(ledCount, ledPin, NEO_GRBW + NEO_KHZ800);  // create neopixel object like you commonly used with Adafruit
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

#include <Noiasca_NeopixelDisplay.h>                                                                               // download library from: http://werner.rothschopf.net/202005_arduino_neopixel_display.htm
Noiasca_NeopixelDisplay display(strip, segment, numDigits, pixelPerDigit, startPixel, addPixels, offsetLogic_cb);  // create display object, handover the name of your strip as first parameter!
/***************************  MQTT *********************************/
#include <PubSubClient.h>  // Allows us to connect to, and publish to the MQTT brokettr
const char *mqtt_topic[2] = { "/Piscine/Temp", "/meteo/sensor/bmp280_temperature/state" };
const char *clientID = "Large_display";  // The client id identifies the ESP device
int maxtopic = 1;  // 2  (0 and 1)
int numeric;  //mqtt string length
int i = 0;
int x = 0;
int topic = 0;
char data[22];
PubSubClient client(mqtt_server, mqtt_port, wifiClient);
/*************************** get data *********************************/
bool Connect() {
  // Connect to MQTT Server and subscribe to the topic
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    for (topic = 0; topic <= maxtopic; topic++) {
      client.subscribe(mqtt_topic[topic]);
    }
    return true;
  } else {
    return false;
  }
}
void callback(char *topic, byte *payload, unsigned int length) {
  String resultat = "";
  if (length > 0) numeric = 5;  // number of characters + 1 // 5, only one decimal at temp eau
  Serial.println(topic);
  do {
    resultat = resultat + String((char)payload[i]);
    if (isdigit((char)payload[0])) numeric = 4;  // only one decimal at outside temp, numeric, use ctype.h
    if (resultat != "?") i++;
  } while (i < numeric);  // print only first digits
  resultat.trim();        // remove spaces
  display.clear();
  if (strcmp(topic, "/Piscine/Temp")) {
    Serial.print("Air ");
    display.setColorFont(GREEN);
    display.print("Air ");
    delay(300);
    display.print("Ai r");
    delay(300);
    display.print("A ir");
    delay(300);
    display.print(" Air");
  } else {
    Serial.println("Eau");
    display.setColorFont(BLUE);
    display.print("EAU ");
    delay(300);
    display.print("EA U");
    delay(300);
    display.print("E AU");
    delay(300);
    display.print(" EAU");
  }
  if (resultat.length() < 4) {  // 3 characters
    resultat = resultat + "  "; // leading space
  }
  delay(1000);
  Serial.println(resultat);
  display.clear();
  display.print(resultat);
  i = 0;
  delay(4000);
  display.clear();
}
/********************* End MQTT *********************************/
void setup() {
  Serial.begin(115200);
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(5);  // Set BRIGHTNESS (max = 255)
  //  display.setRightToLeft();
  display.clear();
  connectWiFi();
  checkForUpdates();
  DisplayTime();
  // Connect to MQTT Broker
  // setCallback sets the function to be called when a message is received.
  client.setCallback(callback);
  if (Connect()) {
    Serial.println("Connected Successfully to MQTT Broker!");
  } else {
    Serial.println("Connection Failed!");
  }
}
/********************* End Setup *********************************/
void loop() /* always 4 characters, else a walking licht */
{
  Serial.print("Version = ");
  Serial.println(FW_VERSION);
  if (up_date > dataPostDelay * 12) {  // twice a day
    checkForUpdates();
    up_date = 0;
//    ESP.restart();
  }
  display.setColorFont(RED);
  sensorValue = analogRead(LDR) + 1;  // ESP32 0-4095
  int Light = int(255 - (sensorValue / 16));
  //  Serial.println(Light);
  if (Light < 2) Light = 2;  // not too dark
  if (Light > 255) Light = 255;
  if (go == 1) {                 // daytime, but after 8 o'clock
    strip.setBrightness(Light);  // Set BRIGHTNESS (max = 255)
  } else {
    strip.setBrightness(0);  // no display between midnight and 8
  }
  DisplayTime();
  if (time_str.length() < 4) {
    time_str = " " + time_str;
  }
  Serial.print("go = ");
  Serial.println(go);
  if (go == 1) {
    //  Serial.println(time_str);    // xx,xx,
    display.print(time_str);
    delay(500);
    DisplayTime();
    time_str.remove(2, 1);
    time_str.remove(4, 1);
    display.print(time_str);
  }
  delay(500);
  if (!client.connected()) {
    connectWiFi();
    Connect();
  }
  up_date++;
  client.loop();
}
