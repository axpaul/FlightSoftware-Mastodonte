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
#include "drv8872.h"

void setup(void) {
  debug_println("[SETUP] Initializing system...");

  adc_init();            // Initialize ADC module
  setup_pin();           // Configure GPIOs as defined in board/config
  setup_rgb();           // Enable onboard RGB
  debug_println("[SETUP] GPIOs, ADC, and RGB initialized.");

  // === Read battery voltage and MCU temperature ===
  float voltage_batt = battery_read_voltage();
  float temperature = temperature_read_mcu();

  if (debug_begin()) {
    debug_println("[BOOT] Mastodonte system ready.");
    debug_printf("[BOOT] Board: %s, FW: %s\n", BOARD_NAME_SYS, FW_VERSION);
    debug_printf("[BOOT] Battery Voltage: %.2f V\n", voltage_batt);
    debug_printf("[BOOT] MCU Temperature: %.2f °C\n", temperature);
    gpio_put(PIN_LED_STATUS, LOW);  // OK indicator
  } else {
    debug_println("[BOOT] Debug interface not initialized!");
    gpio_put(PIN_LED_STATUS, HIGH);  // Error indicator
  }

  // === Motor driver initialization and test ===
  debug_println("[SETUP] Initializing DRV8872 motor drivers...");
  motor_diag_init();
  motor_diag_test_sequence();
  debug_println("[SETUP] Motor test sequence complete.");

  // === Activate all motors (forward then reverse) ===
  debug_println("[SETUP] Activating all motors FORWARD for 3 seconds...");
  group_all_motors.direction = true;
  drv8872_group_activate_for_us(&group_all_motors, 3000000);
  sleep_ms(3500);

  debug_println("[SETUP] Activating all motors REVERSE for 3 seconds...");
  group_all_motors.direction = false;
  drv8872_group_activate_for_us(&group_all_motors, 3000000);
  sleep_ms(3500);

  // === Launch main flight sequencer ===
  debug_println("[SETUP] Initializing flight sequencer...");
  seq_init();
  debug_println("[SETUP] System is operational.");
}


void loop() {
  seq_handle();
}
