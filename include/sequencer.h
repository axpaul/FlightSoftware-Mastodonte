// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#ifndef SEQUENCER_HEADER_FILE
#define SEQUENCER_HEADER_FILE

#define THEORETICAL_APOGEE_US 10000000
#define THEORETICAL_DESCENT_US 15000000
#define WINDOW_MARGIN_PCT     20
#define WINDOW_OPEN_OFFSET_US (THEORETICAL_APOGEE_US * (100 - WINDOW_MARGIN_PCT) / 100)
#define WINDOW_DURATION_US    (2 * THEORETICAL_APOGEE_US * WINDOW_MARGIN_PCT / 100)

#include "platform.h"
#include "debug.h"
#include "buzzer.h"
#include "system.h"
#include "drv8872.h"
#include "log.h"

extern volatile uint8_t triggerRBF;

rocket_state_t seq_init(void);
rocket_state_t seq_handle(void);
void seq_log_event(const char* evt);

rocket_state_t seq_preLaunch(void);
rocket_state_t seq_pyroRdy(void);
rocket_state_t seq_ascend(void);
rocket_state_t seq_window(void);
rocket_state_t seq_deploy(void);
rocket_state_t seq_descend(void);
rocket_state_t seq_touchdown(void);

// Interrupt Fonction 
void seq_gpio_callback(uint gpio, uint32_t events);
int64_t seq_is_window_open_callback(alarm_id_t id, void* user_data);
int64_t seq_window_timeout_callback(alarm_id_t id, void* user_data);
int64_t seq_touchdown_timeout_callback(alarm_id_t id, void* user_data);

void seq_start_window_timer();
void seq_reset_triggers();

void seq_detect_apogee(void);
void seq_deploy_recovery(void);


#endif 