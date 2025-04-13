// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#ifndef SEQUENCER_HEADER_FILE
#define SEQUENCER_HEADER_FILE

#include "board.h"
#include "debug.h"
#include "buzzer.h"
#include "system.h"

typedef enum {PRE_FLIGHT = 0, PYRO_RDY, ASCEND, DEPLOY_ALGO, DEPLOY_TIMER, DESCEND, TOUCHDOWN, ERROR_SEQ} rocket_state_t;

extern volatile uint8_t triggerRBF;

rocket_state_t seq_init(void);
rocket_state_t seq_handle(void);
void seq_log_event(const char* evt);

rocket_state_t seq_preLaunch(void);
rocket_state_t seq_pyroRdy(void);
rocket_state_t seq_ascend(void);
rocket_state_t seq_deploy(void);
rocket_state_t seq_descend(void);
rocket_state_t seq_touchdown(void);

// Interrupt Fonction 
void seq_arm_rbf(void);
void seq_detect_liftoff(void);
void seq_detect_apogee(void);
void seq_is_window_open(void);
void seq_deploy_recovery(void);

void apply_state_config(rocket_state_t state);


#endif