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
#include "system.h"

typedef enum {PRE_FLIGHT = 0, PYRO_RDY, ASCEND, DEPLOY_ALGO, DEPLOY_TIMER, DESCEND, TOUCHDOWN} rocket_state_t;


rocket_state_t seq_init(void);
rocket_state_t seq_handle(void);
void seq_log_event(const char* evt);

rocket_state_t seq_preLaunch(void);
rocket_state_t seq_pyroRdy(void);
rocket_state_t seq_ascend(void);
rocket_state_t seq_deploy(void);
rocket_state_t seq_descend(void);
rocket_state_t seq_touchdown(void);

bool seq_arm_rbf(void);
bool detect_liftoff(void);
bool detect_apogee(void);
bool is_window_open(void);
void deploy_recovery(void);


#endif