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
  setup_interrupt();
  setup_rgb();

  if (debug_begin()) {
    debug_println("[BOOT] Mastodonte ready.");
    debug_printf("[BOOT] Board: %s, FW: %s\n", BOARD_NAME_SYS, FW_VERSION);
    debug_printf("[BOOT] Voltage Battery : %f\n", battery_read_voltage(analogRead(PIN_VCC_BAT)));
    digitalWrite(PIN_LED_STATUS, LOW);
  } else {
    digitalWrite(PIN_LED_STATUS, HIGH);
  }
}

void loop() {

  if (triggerRBF == 1) {
    debug_println("[MAIN] RBF remove");
    apply_state_config(PRE_FLIGHT);
    delay(10000);
    apply_state_config(PYRO_RDY);
    delay(10000);
    apply_state_config(ASCEND);
    delay(10000);
    apply_state_config(DEPLOY_ALGO);
    delay(10000);
    apply_state_config(DESCEND);
    delay(10000);
    apply_state_config(TOUCHDOWN);
    delay(10000);
    apply_state_config(ERROR_SEQ);
    delay(10000);
    apply_state_config(PRE_FLIGHT);
    triggerRBF = 0;
  } else if (triggerRBF == 2) {
    debug_println("[MAIN] RBF inserted");
    apply_state_config(PRE_FLIGHT);
    triggerRBF = 0;
  }



  // Boucle principale sans action si pas de changement
}
