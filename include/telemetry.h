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

#endif // TELEMETRY_HEADER_FILE
