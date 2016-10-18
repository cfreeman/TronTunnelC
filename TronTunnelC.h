#ifndef _TRON_TUNNEL_C_ACH_
#define _TRON_TUNNEL_C_ACH_

typedef struct State_struct (*StateFn)(struct State_struct currentState,
									   int newPosition,
                                       unsigned long currentTime);

typedef struct State_struct {
  uint8_t gHue;				  // rotating "base color" used by many of the patterns
  int position;			      // The current position of an obstacle.
  int targetPosition;		  // The target position of an obstacle.
  unsigned long startedAt;	  // The time the current state started.
  StateFn updateLED;          // The current function to use to when updating the state.
} State;

State idleMode(State currentState, int newPosition, unsigned long currentTime);

State followMode(State currentState, int newPosition, unsigned long currentTime);

State burstMode(State currentState, int newPosition, unsigned long currentTime);

#endif