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

#include "hardware/timer.h"
#include "sequencer.h"
#include "board.h"

typedef struct {
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
    uint32_t microseconds;
  } timestamp_t;

  extern Adafruit_NeoPixel rgb;
  
  timestamp_t compute_timestamp(absolute_time_t timestamp);
  float battery_read_voltage(uint16_t raw);

  void setup_pin(void);
  void setup_interrupt(void);
  void setup_rgb(void);

  #endif