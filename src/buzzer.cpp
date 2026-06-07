// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : buzzer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : passive buzzer control using RP2040 hardware PWM
// -------------------------------------------------------------

#include "buzzer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

static BuzzerConfig buzzerConfig; 

struct repeating_timer buzzerTimer;
volatile bool buzzerEnabled = false;

bool buzzerCallback(struct repeating_timer *t);
int64_t buzzerStopCallback(alarm_id_t id, void *user_data);

static void buzzer_stop() {
  uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
  uint channel = pwm_gpio_to_channel(PIN_BUZZER);
  pwm_set_chan_level(slice_num, channel, 0);
  pwm_set_enabled(slice_num, false);
}

static void buzzer_start(uint16_t frequency) {
  if (frequency == 0) {
    buzzer_stop();
    return;
  }
  uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
  uint channel = pwm_gpio_to_channel(PIN_BUZZER);
  
  uint32_t system_clock_hz = clock_get_hz(clk_sys);
  float clkdiv = 133.0f;
  uint32_t counter_clock_hz = (uint32_t)(system_clock_hz / clkdiv);
  
  uint32_t wrap = (counter_clock_hz / frequency) - 1;
  if (wrap > 65535) wrap = 65535;
  
  pwm_set_wrap(slice_num, wrap);
  pwm_set_chan_level(slice_num, channel, wrap / 2);
  pwm_set_enabled(slice_num, true);
}

void buzzer_init() {
  gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(PIN_BUZZER);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 133.0f); // 133MHz / 133 = 1MHz counter clock
  pwm_init(slice_num, &config, false);   // Initialize but keep disabled
  buzzer_stop();
}

// Active ou désactive le buzzer
void setBuzzer(bool enable, uint16_t beatPeriodMs, uint16_t beatDurationMs, uint16_t frequency) {
  cancel_repeating_timer(&buzzerTimer);
  buzzerEnabled = enable;

  if (!enable) {
    buzzer_stop();
    return;
  }

  if (beatPeriodMs > 0) {
    buzzerConfig.freq = frequency;
    buzzerConfig.duration = beatDurationMs;
    add_repeating_timer_ms(beatPeriodMs, buzzerCallback, &buzzerConfig, &buzzerTimer);
  } else {
    buzzer_start(frequency);
  }
}

bool buzzerCallback(struct repeating_timer *t) {
  if (!buzzerEnabled || !t->user_data) return false;

  BuzzerConfig* config = static_cast<BuzzerConfig*>(t->user_data);
  buzzer_start(config->freq);
  add_alarm_in_ms(config->duration, buzzerStopCallback, nullptr, true);
  return true;
}

int64_t buzzerStopCallback(alarm_id_t, void*) {
  buzzer_stop();
  return 0;
}
