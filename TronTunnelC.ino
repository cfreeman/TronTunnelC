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
WiFiServer server(80);

typedef struct {
  char instruction;
  float argument;
} Command;

void setup() {
  delay(2000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
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

  return (Command) {'p', line.substring(14).toFloat()};
}

void loop() {
  Command c = readCommand();

  if (c.instruction == 'p') {
    Serial.println(c.argument);

    FastLED.show();
  }

  delay(10);
}
