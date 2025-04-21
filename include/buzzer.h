// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : buzzer.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Passive buzzer control (tone or periodic beeping)
// -------------------------------------------------------------

#ifndef BUZZER_HEADER
#define BUZZER_HEADER

#include <Arduino.h>

#include "platform.h"
#include "hardware/timer.h"

// Appelle setBuzzer(true, 1000, 100); pour un bip toutes les secondes pendant 100 ms
void setBuzzer(bool enable, uint16_t beatPeriodMs = 0, uint16_t beatDurationMs = 0, uint16_t frequency = 2000);

// Configuration persistante du buzzer
struct BuzzerConfig {
    uint16_t freq;
    uint16_t duration;
  };

#endif
