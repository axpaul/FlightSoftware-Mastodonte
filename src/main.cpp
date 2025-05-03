// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : main.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Flight software 
// -------------------------------------------------------------

#include <Arduino.h>

#include "pico/multicore.h"

#include "platform.h"
#include "debug.h"
#include "buzzer.h"
#include "sequencer.h"
#include "system.h"
#include "drv8872.h"

//void core1_main();

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
    gpio_put(PIN_LED_STATUS, HIGH);  // OK indicator
  } else {
    debug_println("[BOOT] Debug interface not initialized!");
    gpio_put(PIN_LED_STATUS, LOW);  // Error indicator
  }

  // === Initialisation du système de fichiers log ===
  debug_println("[SETUP] Initializing log system...");
  if (!log_init()) {
    debug_println("[ERROR] Failed to initialize LittleFS log system.");
    apply_state_config(ERROR_SEQ);
  } else {
    // Afficher l'espace total et libre
    FSInfo fs_info;
    LittleFS.info(fs_info);
    debug_printf("[LOG] Used space (approx) : %lu bytes\n", fs_info.usedBytes);
    //multicore_launch_core1(core1_main);
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

  log_entryf("[BOOT] System startup");
  log_entryf("[BOOT] Temperature = %.2f °C", temperature);
  log_entryf("[BOOT] Battery voltage = %.2f V", voltage_batt);

  // Vérification des conditions 
  if (!log_has_space()) {
    log_entry("[BOOT] ERROR: Log memory is full.");
    apply_state_config(ERROR_SEQ);
    return;
  }

  if (temperature > 70.0f) {
    log_entry("[BOOT] ERROR: Temperature too high (> 70°C).");
    apply_state_config(ERROR_SEQ);
    return;
  }

  if (voltage_batt <= 6.0f) {
    log_entry("[BOOT] ERROR: Battery voltage too low (<= 6V).");
    apply_state_config(ERROR_SEQ);
    return;
  }

  debug_println("[SETUP] Initializing flight sequencer...");
  seq_init();
  debug_println("[SETUP] System is operational");

}

void loop() {
  seq_handle();
  log_flush();
}

// void core1_main() {
//   while (true) {
//       log_flush();  // écrit les messages si présents
//       sleep_ms(10); // évite de saturer le CPU
//   }
// }