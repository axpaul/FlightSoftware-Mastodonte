// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : main.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Flight software 
// -------------------------------------------------------------

#include <Arduino.h>

#include "board.h"
#include "debug.h"
#include "buzzer.h"
#include "sequencer.h"
#include "system.h"

void setup(void) {

  analogReadResolution(12);
  setup_pin();
  setup_rgb();

  if (debug_begin()) {
    debug_println("[BOOT] Mastodonte ready.");
    debug_printf("[BOOT] Board: %s, FW: %s\n", BOARD_NAME_SYS, FW_VERSION);
    debug_printf("[BOOT] Voltage Battery : %f\n", battery_read_voltage(analogRead(PIN_VCC_BAT)));
    digitalWrite(PIN_LED_STATUS, LOW);
  } else {
    digitalWrite(PIN_LED_STATUS, HIGH);
  }

  seq_init();

}

void loop() {
  seq_handle();
}
