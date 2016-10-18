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

extern "C" {
  #include "user_interface.h"
}

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

// Underlying hardware.
WiFiUDP udp;
unsigned int udpPort = 4210;
IPAddress masterIP(192,168,4,1);
os_timer_t renderTimer;

// the position as recieved from master
int16_t pos = -1; // position default to "none"

// add a little smoothing for what has been RXed from AP
const int numDistanceReadings = 20;
int16_t distances[numDistanceReadings] = {0};
int16_t distanceReadIndex = 0;
int16_t distanceTotal = 0;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void render(void *arg) {
  EVERY_N_MILLISECONDS( 20 ) {gHue++;ledEffects.setHue(gHue);}
  ledEffects.dotFadeColourWithRainbowSparkle(leds, smooth(pos), CRGB::White);
  FastLED.show();
}

void setup() {
  delay(2000);  // Give the sensor / master ESP8266 a couple of seconds head start.
  Serial.begin(9600);

  // Added to prevent having to power cycle after upload.
  WiFi.disconnect();

  // Connect to the Access Point / sensor / master ESP8266.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");

  udp.begin(udpPort);

  // Setup the leds with the correct colour order and a little colour correction (its better than it was...)
  FastLED.addLeds<LED_TYPE,DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(0x80B0FF);
  FastLED.setBrightness(BRIGHTNESS);

  // Rendering has a fixed framerate of 60fps, see the above 'render' function.
  os_timer_disarm(&renderTimer);
  os_timer_setfn(&renderTimer, render, &pos);
  os_timer_arm(&renderTimer, 17, true);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (!packetSize) {
    return;
  }

  char incomingPacket[255];
  int len = udp.read(incomingPacket, 255);
  if (len > 0) {
    incomingPacket[len] = 0;
  }

  pos = NUM_LEDS - (int16_t)(String(incomingPacket).toFloat() * NUM_LEDS);

  // Read a new position from the sensor every 40 milliseconds.
  // NOTE: Rendering occurs at a fixed interval in the timer interupt above.
  delay(40);
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
