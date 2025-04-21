// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : main.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Flight software 
// -------------------------------------------------------------

#include <Arduino.h>

#include "platform.h"
#include "debug.h"
#include "buzzer.h"
#include "sequencer.h"
#include "system.h"

void setup(void) {
  adc_init();            // Initialise le module ADC
  setup_pin();           // Initialise les GPIO définies dans board/config
  setup_rgb();           // Active le RGB embarqué

  float voltage_batt = battery_read_voltage();   // Lecture tension batterie
  float temperature = temperature_read_mcu();     // Lecture température interne RP2040

  if (debug_begin()) {
    debug_println("[BOOT] Mastodonte ready.");
    debug_printf("[BOOT] Board: %s, FW: %s\n", BOARD_NAME_SYS, FW_VERSION);
    debug_printf("[BOOT] Voltage Battery : %.2f V\n", voltage_batt);
    debug_printf("[BOOT] MCU Temperature : %.2f °C\n", temperature);
    gpio_put(PIN_LED_STATUS, LOW); 
  } else {
    gpio_put(PIN_LED_STATUS, HIGH); 
  }

  seq_init(); 
}


void loop() {
  seq_handle();
}
