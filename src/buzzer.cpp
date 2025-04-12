// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : buzzer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Passive buzzer control (tone or periodic beeping)
// -------------------------------------------------------------

#include "buzzer.h"

static BuzzerConfig buzzerConfig; // Ne plus utiliser new/delete

struct repeating_timer buzzerTimer;
volatile bool buzzerEnabled = false;

bool buzzerCallback(struct repeating_timer *t);
int64_t buzzerStopCallback(alarm_id_t id, void *user_data);

// Active ou désactive le buzzer
void setBuzzer(bool enable, uint16_t beatPeriodMs, uint16_t beatDurationMs, uint16_t frequency) {
  cancel_repeating_timer(&buzzerTimer);
  buzzerEnabled = enable;

  if (!enable) {
    noTone(PIN_BUZZER);
    return;
  }

  if (beatPeriodMs > 0) {
    buzzerConfig.freq = frequency;
    buzzerConfig.duration = beatDurationMs;
    add_repeating_timer_ms(beatPeriodMs, buzzerCallback, &buzzerConfig, &buzzerTimer);
  } else {
    tone(PIN_BUZZER, frequency);
  }
}

bool buzzerCallback(struct repeating_timer *t) {
  if (!buzzerEnabled || !t->user_data) return false;

  BuzzerConfig* config = static_cast<BuzzerConfig*>(t->user_data);
  tone(PIN_BUZZER, config->freq);
  add_alarm_in_ms(config->duration, buzzerStopCallback, nullptr, true);
  return true;
}

int64_t buzzerStopCallback(alarm_id_t, void*) {
  noTone(PIN_BUZZER);
  return 0;
}
