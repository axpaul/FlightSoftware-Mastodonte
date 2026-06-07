// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : telemetry.h
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Module de suivi des statistiques de vol et du rapport final
// -------------------------------------------------------------

#ifndef TELEMETRY_HEADER_FILE
#define TELEMETRY_HEADER_FILE

#include "pico/stdlib.h"

/**
 * @brief Initialise les statistiques de vol au décollage (LIFTOFF).
 */
void telemetry_init(void);

/**
 * @brief Met à jour les statistiques de vol (à appeler à chaque cycle à 25 Hz).
 */
void telemetry_update(void);

/**
 * @brief Écrit le rapport de vol final formaté dans /log.txt.
 */
void telemetry_save_report(void);

float telemetry_get_max_velocity(void);
float telemetry_get_max_acceleration(void);
float telemetry_get_apogee_altitude(void);
uint32_t telemetry_get_apogee_time_ms(void);
bool telemetry_is_apogee_recorded(void);

#endif // TELEMETRY_HEADER_FILE
