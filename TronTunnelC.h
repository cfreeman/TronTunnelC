#ifndef _TRON_TUNNEL_C_ACH_
#define _TRON_TUNNEL_C_ACH_

typedef struct {
  int intensity;             // The LED intensity at the key frame value range: [0,255]
  unsigned long t;           // The time when this intensity is reached.
} KeyFrame;

typedef struct {
  int pin;                   // The IO pin that the LED is connected too.
  boolean on;                // Is the LED on?

  boolean powerUpProcessed;  // Has the powerup been proccessed?

  KeyFrame start_low;        // When the LED pulse begins.
  KeyFrame start_high;       // When the LED pulse reaches its maxima.
  KeyFrame end_high;         // When the LED pulse departs its maxima.
  KeyFrame end_low;          // When the LED pulse ends.
} LED;

#endif