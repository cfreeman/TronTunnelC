/*
 * Copyright (c) Clinton Freeman & Kurt Schoenhoff 2016
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "ESP8266WiFi.h"
#include "FastLED.h"
#include "FastLed_Effects.h"
#include "TronTunnelC.h"
#include "WiFiUDP.h"

#define DATA_PIN            5
#define CLOCK_PIN           4
#define LED_TYPE            APA102
#define COLOR_ORDER         BGR
#define NUM_LEDS            120
#define BRIGHTNESS          400
#define FRAMES_PER_SECOND   120

// Credentials of the parent Wifi Access Point (AP).
const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";

//construct led array and our efffects class
CRGB leds[NUM_LEDS];
FastLed_Effects ledEffects(NUM_LEDS);

WiFiUDP udp;
unsigned int udpPort = 4210;
IPAddress masterIP(192,168,4,1);

// the position as recieved from master
int16_t pos = -1; // position default to "none"

int8_t readCommandLoopIterator = 0;

// add a little smoothing for what has been RXed from AP
const int numDistanceReadings = 20;
int16_t distances[numDistanceReadings];
int16_t distanceReadIndex = 0;
int16_t distanceTotal = 0;

typedef struct {
  char instruction;
  float argument;
} Command;

void setup() {
  delay(2000);
  Serial.begin(9600);

  //setup smoothing array
  for (int thisDistReading = 0; thisDistReading < numDistanceReadings; thisDistReading++) {
    distances[thisDistReading] = 0;
  }

  //Added to prevent having to power cycle after upload
  WiFi.disconnect();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");

  udp.begin(udpPort);

  // setup the leds with the correct colour order and a little colour correction (its better than ot was...)
  FastLED.addLeds<LED_TYPE,DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(0x80B0FF);
  FastLED.setBrightness(BRIGHTNESS);
}

Command readCommand() {
  int packetSize = udp.parsePacket();
  if (packetSize) {

    // receive incoming UDP packets
    char incomingPacket[255];  // buffer for incoming packets
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }

    return (Command) {'p', String(incomingPacket).toFloat()};
  }

  return (Command) {'*', 0.0};
}

Command c;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop() {
  // only query wifi connection for data after updating the leds a few times
  readCommandLoopIterator++;
  if (readCommandLoopIterator > 6) {
    c = readCommand();
    readCommandLoopIterator = 0;
  }

  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS( 20 ) { gHue++; ledEffects.setHue(gHue);}

  //check what my instructons are
  if (c.instruction == 'p') {
    Serial.println(c.argument);
    pos = (int16_t)(c.argument * NUM_LEDS ) ;
    c.instruction = '0';
  }

  // get what the leds should be
  ledEffects.dotFadeColourWithRainbowSparkle(leds,  smooth(pos), CRGB::White);

  // update the strip
  FastLED.show();
  FastLED.delay(10);
}


// this looks after the smoothing of positional information to allow a nicer transition between positions
int16_t smooth(int16_t value) {
  distanceTotal = distanceTotal - distances[distanceReadIndex];
  distances[distanceReadIndex] = value;
  distanceTotal = distanceTotal + distances[distanceReadIndex];
  distanceReadIndex = distanceReadIndex + 1;

  if (distanceReadIndex >= numDistanceReadings - 1) {
    distanceReadIndex = 0;
  }

  return (distanceTotal / numDistanceReadings);
}
