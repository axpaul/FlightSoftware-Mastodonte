// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#ifndef SEQUENCER_HEADER_FILE
#define SEQUENCER_HEADER_FILE

#include <Arduino.h>
#include <pico/time.h>
#include "board.h"
#include "buzzer.h"

typedef enum {PRE_FLIGHT = 0, PYRO_RDY, ASCEND, DEPLOY_ALGO, DEPLOY_TIMER, DESCEND, TOUCHDOWN} RocketState_t;

RocketState_t seqPreLaunch();
RocketState_t seqPyroRdy();
RocketState_t seqAscend();
RocketState_t seqDeploy();
RocketState_t seqDescend();
RocketState_t seqTouchdown();


#endif