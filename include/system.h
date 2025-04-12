// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#ifndef SYSTEM_HEADER_FILE
#define SYSTEM_HEADER_FILE

#include "hardware/timer.h"

typedef struct {
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
    uint32_t microseconds;
  } timestamp_t;
  
  timestamp_t compute_timestamp(absolute_time_t timestamp);

#endif