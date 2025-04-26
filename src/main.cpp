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

  adc_init();            // Initialize ADC module
  setup_pin();           // Configure GPIOs as defined in board/config
  setup_rgb();           // Enable onboard RGB

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

  // === Initialisation du système de fichiers log ===
  debug_println("[SETUP] Initializing log system...");
  if (!log_init()) {
    debug_println("[ERROR] Failed to initialize LittleFS log system.");
    apply_state_config(ERROR_SEQ);
  } else {
    log_entry("[BOOT] Logging system initialized.");
    // Afficher l'espace total et libre
    FSInfo fs_info;
    LittleFS.info(fs_info);
    debug_printf("[LOG] Used space (approx) : %lu bytes\n", fs_info.usedBytes);
  }

  debug_println("[BOOT] Checking GP24 state...");

  // === Gestion du bouton GP24 ===

  absolute_time_t t_start = get_absolute_time();

  if (gpio_get(24) == 0) {
    debug_println("[BOOT] GP24 is LOW → Dumping logs now...");
    log_dump(); // Dump immédiatement

    // Ensuite, on attend pour voir s’il est maintenu 5s
    absolute_time_t t_start = get_absolute_time();
    while (gpio_get(24) == 0) {
      if (absolute_time_diff_us(t_start, get_absolute_time()) > 5 * 1000 * 1000) {
        debug_println("[BOOT] GP24 held LOW for 5s → Clearing log!");
        log_clear();
        break;
      }
      sleep_ms(50);
    }
  }
  

// === Launch main flight sequencer ===
  debug_println("[SETUP] Initializing flight sequencer...");
  seq_init();
  debug_println("[SETUP] System is operational.");
}


void loop() {
  seq_handle();
}
