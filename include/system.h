// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Gestion matérielle générale et monitoring système
// -------------------------------------------------------------

#ifndef SYSTEM_HEADER_FILE
#define SYSTEM_HEADER_FILE

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/sync.h"    // pour __wfi()

#include "buzzer.h"
#include "platform.h"

/**
 * @brief Structure représentant un horodatage précis découpé.
 */
typedef struct {
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
    uint32_t microseconds;
} timestamp_t;

// Instance globale de la LED RGB WS2812B
extern Adafruit_NeoPixel rgb;

// Variable de l'état actuel partagée par le séquenceur
extern rocket_state_t currentState;

/**
 * @brief Calcule un timestamp découpé à partir d'un temps système absolute_time_t.
 */
timestamp_t compute_timestamp(absolute_time_t timestamp);

/**
 * @brief Lit la tension de la batterie via l'ADC.
 * @return La tension mesurée en Volts.
 */
float battery_read_voltage(void);

/**
 * @brief Lit la température interne du MCU RP2040.
 * @return La température en degrés Celsius.
 */
float temperature_read_mcu(void);

/**
 * @brief Configure les broches GPIO (directions et pull-up/pull-down).
 */
void setup_pin(void);

/**
 * @brief Configure les interruptions du système.
 */
void setup_interrupt(void);

/**
 * @brief Initialise la LED RGB WS2812B intégrée.
 */
void setup_rgb(void);

/**
 * @brief Applique la configuration visuelle (LED) et sonore (Buzzer) selon l'état du vol.
 */
void apply_state_config(rocket_state_t state);

/**
 * @brief Récupère la couleur associée à un état de vol donné.
 */
uint32_t get_state_color(rocket_state_t state);

/**
 * @brief Initialise le timer de monitoring batterie (période 1 seconde).
 */
void system_battery_monitor_init(void);

/**
 * @brief Gère la mesure de tension et le clignotement d'alerte dans la boucle principale.
 */
void system_battery_check_tick(void);

#endif // SYSTEM_HEADER_FILE