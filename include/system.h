// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#ifndef SYSTEM_HEADER_FILE
#define SYSTEM_HEADER_FILE

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/sync.h"    // pour __wfi()

#include "buzzer.h"
#include "platform.h"

typedef struct {
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
    uint32_t microseconds;
  } timestamp_t;


extern Adafruit_NeoPixel rgb;
  
timestamp_t compute_timestamp(absolute_time_t timestamp);

float battery_read_voltage(void);
float temperature_read_mcu(void);

void setup_pin(void);
void setup_interrupt(void);
void setup_rgb(void);

void apply_state_config(rocket_state_t state);

// === Battery monitoring & LED state functions ===
extern rocket_state_t currentState;

uint32_t get_state_color(rocket_state_t state);
void system_battery_monitor_init(void);
void system_battery_check_tick(void);

#endif