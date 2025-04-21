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

// void buzzer_touchdown_loop(uint16_t freq = 400, uint16_t duration = 500, uint32_t period_ms = 60000) {
//   while (true) {
//     // Log (si tu veux)
//     debug_println("[BUZZER] Touchdown mode: beep once then sleep...");

//     // Beep une fois
//     tone(PIN_BUZZER, freq);
//     sleep_ms(duration);
//     noTone(PIN_BUZZER);

//     // Entrée en sommeil profond pour X secondes
//     absolute_time_t wake_time = make_timeout_time_ms(period_ms - duration);
//     debug_printf("[BUZZER] Sleeping for %.1f s\n", (period_ms - duration) / 1000.0f);
//     sleep_goto_dormant_until(wake_time);

//     // À la sortie de veille → boucle recommence
//   }
// }