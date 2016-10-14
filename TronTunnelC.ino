/*
 * Copyright (c) Clinton Freeman 2016
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
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#include "ESP8266WiFi.h"
#include "FastLED.h"
#include "TronTunnelC.h"


#define NUM_LEDS 300

#define DATA_PIN 3
#define CLOCK_PIN 4

const char* ssid = "tron-tunnel";
const char* password = "tq9Zjk23";

CRGB leds[NUM_LEDS];

typedef struct {
  char instruction;
  float argument;
} Command;

WiFiServer server(80);

void setup() {
  delay(2000);
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected!");

  server.begin();

  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB, DATA_RATE_MHZ(10)>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
}

Command readCommand() {
  // No clients, return empty command.
  WiFiClient client = server.available();
  if (!client) {
    return (Command) {'*', 0.0};
  }

  // No data from the client, return empty command.
  if (client.available() < 8) {
    return (Command) {'*', 0.0};
  }

  String line = client.readStringUntil('\r');

  // Tell the client we got the request.
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!doctype html><title>.</title>");
  client.stop();

  // Unknown message format, return empty command
  if (line.indexOf("/update?p=") == -1) {
    return (Command) {'*', 0.0};
  }

  Serial.println(line.substring(14));

  return (Command) {'p', line.substring(14).toFloat()};
}

void loop() {
  Command c = readCommand();

  // Serial.print(c.instruction);
  // Serial.print(" ");
  // Serial.println(c.argument);
  // Pulse the trigger pin and wait for an echo.
  // digitalWrite(TRIGGER, HIGH);
  // delayMicroseconds(10);
  // digitalWrite(TRIGGER, LOW);

  // long duration = pulseIn(ECHO, HIGH);
  // float distance = min(200.0, (duration/2) / 29.1);

  // Serial.print(distance);
  // Serial.println(" cm");
  for(int i = 0; i < 120; i++) {
    leds[i] = CRGB(255, 255, 255);
  }


  for(int i = 120; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }

  FastLED.show();
  delay(10);
}

