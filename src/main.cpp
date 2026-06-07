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
#include "lps22hb.h"
#include "lsm6dsl.h"

// Callback globale d'interruption GPIO (Aiguilleur central)
void main_gpio_callback(uint gpio, uint32_t events) {
  // Acknowledge l'interruption immédiatement pour libérer le matériel
  gpio_acknowledge_irq(gpio, events);

  // 1. Aiguillage vers les moteurs si c'est un défaut nFAULT
  if (gpio == NFAUT_M1 || gpio == NFAUT_M2 || gpio == NFAUT_M3) {
    drv8872_handle_fault(gpio);
  }
  // 2. Aiguillage vers le séquenceur de vol si c'est un événement physique de vol
  else if (gpio == PIN_SMITCH_N1 || gpio == PIN_SMITCH_N2 || gpio == PIN_OCTO_N3 || gpio == PIN_OCTO_N4) {
    seq_gpio_callback(gpio, events);
  }
  // 3. Aiguillage vers la routine de filtrage du baromètre (Data Ready)
  else if (gpio == PIN_BARO_INT) {
    lps22hb_drdy_callback();
  }
}

void setup(void) {
  // === Initialisation matérielle de base ===
  adc_init();            // Initialise le module ADC
  setup_pin();           // Configure les GPIOs physiques (platform.h)
  system_i2c_init();     // Configure et démarre le bus I2C1 (Berry MiniSensor)
  setup_rgb();           // Active et configure la LED RGB intégrée (WS2812B)
  buzzer_init();         // Initialise le buzzer matériel (PWM)

  // Enregistre l'aiguilleur GPIO global auprès du processeur
  gpio_set_irq_callback(main_gpio_callback);

  // === Initialisation des moteurs ===
  drv8872_init(&motor1);
  drv8872_init(&motor2);
  drv8872_init(&motor3);
  drv8872_setup_fault_interrupt(&motor1);
  drv8872_setup_fault_interrupt(&motor2);
  drv8872_setup_fault_interrupt(&motor3);

  // === Lecture initiale de la batterie et de la température ===
  float voltage_batt = battery_read_voltage();
  float temperature = temperature_read_mcu();

  // === Initialisation de l'interface de debug ===
  bool serial_ready = debug_begin();

  // === Initialisation du système de fichiers log ===
  bool log_fs_ok = log_init();
  FSInfo fs_info;
  if (log_fs_ok) {
    LittleFS.info(fs_info);
  }

  // === Gestion du bouton utilisateur GP24 (Dump / Effacement des logs) ===
  if (gpio_get(24) == 0) {
    if (serial_ready) {
      debug_println("[BOOT] Bouton GP24 actif (LOW) -> Dump des logs en cours...");
    }
    log_dump(); // Affiche immédiatement les logs sur le port série

    // Surveillance de l'appui maintenu (5 secondes) pour effacer les logs
    absolute_time_t t_start = get_absolute_time();
    while (gpio_get(24) == 0) {
      if (absolute_time_diff_us(t_start, get_absolute_time()) > 5 * 1000 * 1000) {
        if (serial_ready) {
          debug_println("[BOOT] Bouton GP24 maintenu 5s -> Effacement des logs !");
        }
        log_clear();
        break;
      }   
      sleep_ms(50);
    }
  }

  // === Initialisation des capteurs I2C et vérification de leur présence ===
  bool baro_ok = lps22hb_init();
  bool imu_ok = lsm6dsl_init();

  if (baro_ok) {
    // Émission de deux bips courts pour confirmer l'initialisation réussie du baromètre
    setBuzzer(true, 0, 0, 1500);
    sleep_ms(80);
    setBuzzer(false, 0, 0, 0);
    sleep_ms(80);
    setBuzzer(true, 0, 0, 1500);
    sleep_ms(80);
    setBuzzer(false, 0, 0, 0);

    // Initialisation de la broche d'interruption du baromètre (GP5)
    gpio_init(PIN_BARO_INT);
    gpio_set_dir(PIN_BARO_INT, GPIO_IN);
    gpio_disable_pulls(PIN_BARO_INT);

    // Enregistre l'interruption matérielle sur front montant (DRDY)
    gpio_set_irq_enabled(PIN_BARO_INT, GPIO_IRQ_EDGE_RISE, true);
    log_entry("[BOOT] Baro DRDY interrupt configured on GP5");
  } else {
    // Émission de trois bips pour signaler l'absence de baromètre (mode fenêtrage uniquement)
    setBuzzer(true, 0, 0, 1200);
    sleep_ms(150);
    setBuzzer(false, 0, 0, 0);
    sleep_ms(100);
    setBuzzer(true, 0, 0, 1200);
    sleep_ms(150);
    setBuzzer(false, 0, 0, 0);
    sleep_ms(100);
    setBuzzer(true, 0, 0, 1200);
    sleep_ms(150);
    setBuzzer(false, 0, 0, 0);

    log_entry("[BOOT] Barometre non detecte. Mode FENETRAGE UNIQUEMENT active.");
  }

  // Lancement du monitoring de la batterie en tâche de fond (interruption)
  system_battery_monitor_init();

  // === Écriture des logs de boot ===
  log_entryf("[BOOT] Demarrage systeme");
  log_entryf("[BOOT] Temperature = %.2f C", temperature);
  log_entryf("[BOOT] Tension Batterie = %.2f V", voltage_batt);
  if (!baro_ok || !imu_ok) {
    log_entryf("[BOOT] WARNING: Probleme capteur(s) I2C (Baro=%s, IMU=%s)", baro_ok ? "OK" : "NOK", imu_ok ? "OK" : "NOK");
  }

  // === Affichage du tableau de diagnostic série structuré ===
  if (serial_ready) {
    debug_println("\n==================================================");
    debug_println("        FLIGHT SOFTWARE - MASTODONTE " FW_VERSION);
    debug_println("==================================================");
    debug_printf("[BOOT] Carte : %s\n", BOARD_NAME_SYS);
    debug_printf("[BOOT] Tension Batterie : %.2f V\n", voltage_batt);
    debug_printf("[BOOT] Temperature MCU  : %.2f °C\n", temperature);
    debug_println("--------------------------------------------------");
    if (log_fs_ok) {
      debug_printf("[BOOT] Log System (LittleFS) : OK (Used: %lu B)\n", fs_info.usedBytes);
    } else {
      debug_println("[BOOT] Log System (LittleFS) : FAILED !");
    }
    debug_println("[BOOT] Battery Monitor       : OK");
    debug_println("--------------------------------------------------");
    debug_println("[BOOT] Autotest Actif Moteurs :");
  }

  // Autotest des moteurs (imprime ses résultats compacts)
  bool motors_ok = motor_diag_run_self_test();

  if (serial_ready) {
    debug_printf("       -> Autotest %s.\n", motors_ok ? "reussi" : "ECHOUE");
    debug_println("--------------------------------------------------");
    debug_println("[BOOT] Capteurs I2C (i2c1 @ 400kHz) :");
    if (baro_ok) {
      float press = lps22hb_read_pressure();
      debug_printf("       - Barometre (LPS22HB) : OK (Pression : %.2f hPa)\n", press);
    } else {
      debug_println("       - Barometre (LPS22HB) : FAILED ! (Mode FENETRAGE UNIQUEMENT active)");
    }
    if (imu_ok) {
      float ax = 0.0f, ay = 0.0f, az = 0.0f;
      lsm6dsl_read_accel(&ax, &ay, &az);
      debug_printf("       - IMU (LSM6DSL)       : OK (Accel : X=%.2fg, Y=%.2fg, Z=%.2fg)\n", ax, ay, az);
    } else {
      debug_println("       - IMU (LSM6DSL)       : FAILED !");
    }
    debug_println("--------------------------------------------------");
  }

  if (!motors_ok) {
    log_entry("[BOOT] ERROR: Defaut moteur detecte au demarrage.");
    if (serial_ready) {
      debug_println("[BOOT] ERROR: Defaut moteur detecte au demarrage ! Blocage de securite.");
      debug_println("==================================================");
    }
    apply_state_config(ERROR_MOTOR);
    currentState = ERROR_MOTOR;
    return;
  }

  // Initialisation et démarrage de la machine d'état de vol
  seq_init(); // configure l'état initial PRE_FLIGHT

  if (serial_ready) {
    uint8_t jackState = gpio_get(PIN_SMITCH_N1);
    uint8_t rbfState  = gpio_get(PIN_SMITCH_N2);
    uint8_t octo3State = gpio_get(PIN_OCTO_N3);
    uint8_t octo4State = gpio_get(PIN_OCTO_N4);

    debug_println("[BOOT] Machine d'Etat (FSM) :");
    debug_println("       - Etat Initial : PRE_FLIGHT");
    debug_printf("       - Broche Jack  : %d (%s)\n", jackState, (jackState == 0 ? "REMOVED" : "CONTINUITY"));
    debug_printf("       - Broche RBF   : %d (%s)\n", rbfState, (rbfState == 0 ? "REMOVED" : "INSERTED"));
    debug_printf("       - Optos 3 / 4  : %d / %d (LOW)\n", octo3State, octo4State);
    debug_printf("       - Timers       : Apogee %.2fs | Fenetre %.2fs (duree %.2fs)\n", 
                 (float)THEORETICAL_APOGEE_US / 1e6, (float)WINDOW_OPEN_OFFSET_US / 1e6, (float)WINDOW_DURATION_US / 1e6);
    debug_println("--------------------------------------------------");
    debug_println("[BOOT] Systeme operationnel et pret pour le vol !");
    debug_println("==================================================");
  }
}

void loop() {
  seq_handle();                // Exécute la logique de la machine d'état de vol
  system_battery_check_tick(); // Vérifie périodiquement la tension de la batterie
  log_flush();                 // Écrit les logs du buffer en flash de manière non bloquante
}
