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
#define HALF_STEP_WIDTH     15  // The width of a step in number of LEDs.
#define TUNNEL_START        0   // When 'pos' is greater than this number we will enter step mode.
#define TUNNEL_END          100 // When 'pos' is greater than this number we will enter burst mode.

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

// The current state of our tron tunnel.
State state;

// the position as recieved from master sensor
int pos = -1; // position default to "none"

State idleMode(State currentState,
               int newPosition,
               unsigned long currentTime) {

  // If we have detected a person - switch to follow mode.
  if (newPosition > TUNNEL_START) {
    return {0, TUNNEL_START, newPosition, currentTime, &stepMode};
  }

  // No change - keep animating our noise pattern.
  ledEffects.noise(leds);

  return currentState;
}

State stepMode(State currentState,
               int newPosition,
               unsigned long currentTime) {

  // Person has reached the end of the tunnel - burst instead.
  if (newPosition > TUNNEL_END) {
    return {0, currentState.position, newPosition, currentTime, &burstMode};
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    // Random noise pattern outside the step.
    if (leds[i].r == 0 && random16() < 200) {
      leds[i] = CRGB(40, 40, 40);

    // Illuminate LEDs inside the step at a faster and brighter rate.
    } else if (leds[i].r == 0 && random16() < 1000 && i > (newPosition - HALF_STEP_WIDTH) && i < (newPosition + HALF_STEP_WIDTH)) {
      leds[i] = CRGB(100, 100, 100);

    // Only fade LEDs outside current step.
    } else if (leds[i].r != 0 && (i < (newPosition - HALF_STEP_WIDTH) || i > (newPosition + HALF_STEP_WIDTH))) {
      fadeToBlackBy(&leds[i], 1, random8(5));
    }
  }

  return currentState;
}

State burstMode(State currentState,
                int newPosition,
                unsigned long currentTime) {

  // Full blast when we first burst.
  if ((currentTime - currentState.startedAt) < 20) {
    for (int i = 0; i < NUM_LEDS; i++) {
      int intensity = random8(100, 255);
      leds[i] = CRGB(intensity, intensity, intensity);
    }
  }

  ledEffects.noise(leds);

  // Return to idle mode after have cooled down.
  unsigned long dt = currentTime - currentState.startedAt;
  if (dt > 4000) {
    Serial.println("idle");
    return {0, 0, 0, currentTime, &idleMode};
  }

  return currentState;
}

void render(void *arg) {

  state = state.updateLED(state, pos, millis());
  FastLED.show(); // Render our LED changes to the hardware.
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

  state = {0, 0, 0, millis(), &burstMode};

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

  // disable interupts and dump in update.
  //os_timer_disarm(&renderTimer);
  pos = NUM_LEDS - (int)(String(incomingPacket).toFloat() * NUM_LEDS);
  //os_timer_arm(&renderTimer, 17, true);

  // Read a new position from the sensor every 40 milliseconds.
  // NOTE: Rendering occurs at a fixed interval in the timer interupt above.
  delay(40);
}
