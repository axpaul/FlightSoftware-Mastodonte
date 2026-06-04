// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : main.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Logiciel de vol principal - Initialisation et boucle de contrôle
// -------------------------------------------------------------

#include <Arduino.h>
#include "pico/multicore.h"
#include "platform.h"
#include "debug.h"
#include "buzzer.h"
#include "sequencer.h"
#include "system.h"
#include "drv8872.h"

void setup(void) {
  // === Initialisation matérielle de base ===
  adc_init();            // Initialise le module ADC
  setup_pin();           // Configure les GPIOs physiques (platform.h)
  setup_rgb();           // Active et configure la LED RGB intégrée (WS2812B)

  // === Lecture initiale de la batterie et de la température ===
  float voltage_batt = battery_read_voltage();
  float temperature = temperature_read_mcu();

  // === Initialisation de l'interface de debug ===
  if (debug_begin()) {
    debug_println("[BOOT] Systeme Mastodonte pret.");
    debug_printf("[BOOT] Carte : %s, FW : %s\n", BOARD_NAME_SYS, FW_VERSION);
    debug_printf("[BOOT] Tension Batterie : %.2f V\n", voltage_batt);
    debug_printf("[BOOT] Temperature MCU : %.2f °C\n", temperature);
    gpio_put(PIN_LED_STATUS, HIGH);  // Indicateur d'état OK (LED verte RP2040)
  } else {
    debug_println("[BOOT] Interface de debug non initialisee !");
    gpio_put(PIN_LED_STATUS, LOW);   // Indicateur d'état Erreur
  }

  // === Initialisation du système de fichiers log ===
  debug_println("[SETUP] Initialisation du systeme de fichiers de log...");
  if (!log_init()) {
    debug_println("[ERROR] Echec de l'initialisation du systeme de log LittleFS.");
    apply_state_config(ERROR_SEQ);
  } else {
    // Lecture et affichage des statistiques de l'espace de stockage
    FSInfo fs_info;
    LittleFS.info(fs_info);
    debug_printf("[LOG] Espace utilise (approx) : %lu octets\n", fs_info.usedBytes);
  }

  // === Gestion du bouton utilisateur GP24 (Dump / Effacement des logs) ===
  debug_println("[BOOT] Verification de l'etat du bouton GP24...");
  if (gpio_get(24) == 0) {
    debug_println("[BOOT] Bouton GP24 actif (LOW) -> Dump des logs en cours...");
    log_dump(); // Affiche immédiatement les logs sur le port série

    // Surveillance de l'appui maintenu (5 secondes) pour effacer les logs
    absolute_time_t t_start = get_absolute_time();
    while (gpio_get(24) == 0) {
      if (absolute_time_diff_us(t_start, get_absolute_time()) > 5 * 1000 * 1000) {
        debug_println("[BOOT] Bouton GP24 maintenu 5s -> Effacement des logs !");
        log_clear();
        break;
      }   
      sleep_ms(50);
    }
  }

  // === Démarrage du séquenceur de vol principal ===
  log_entryf("[BOOT] Demarrage systeme");
  log_entryf("[BOOT] Temperature = %.2f C", temperature);
  log_entryf("[BOOT] Tension Batterie = %.2f V", voltage_batt);

  // Vérification de la capacité restante dans la flash
  if (!log_has_space()) {
    log_entry("[BOOT] ERROR: Memoire flash de log pleine.");
    apply_state_config(ERROR_SEQ);
    return;
  }

  // Alerte non bloquante en cas de batterie faible au démarrage
  if (voltage_batt <= 6.0f) {
    log_entry("[BOOT] WARNING: Tension batterie faible (<= 6V).");
    debug_println("[BOOT] WARNING: Tension batterie faible !");
  }

  // Lancement du monitoring de la batterie en tâche de fond (interruption)
  debug_println("[SETUP] Initialisation du monitoring batterie...");
  system_battery_monitor_init();

  // Initialisation et démarrage de la machine d'état de vol
  debug_println("[SETUP] Initialisation du sequenceur de vol...");
  seq_init();
  debug_println("[SETUP] Systeme operational");
}

void loop() {
  seq_handle();                // Exécute la logique de la machine d'état de vol
  system_battery_check_tick(); // Vérifie périodiquement la tension de la batterie
  log_flush();                 // Écrit les logs du buffer en flash de manière non bloquante
}
