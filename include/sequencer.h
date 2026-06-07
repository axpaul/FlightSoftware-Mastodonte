// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Machine d'état du séquenceur de vol de la fusée
// -------------------------------------------------------------

#ifndef SEQUENCER_HEADER_FILE
#define SEQUENCER_HEADER_FILE

// Constantes temporelles et marges de vol (en microsecondes)
#define THEORETICAL_APOGEE_US 17000000
#define THEORETICAL_DESCENT_US 60000000
#define WINDOW_MARGIN_PCT     15
#define WINDOW_OPEN_OFFSET_US (THEORETICAL_APOGEE_US * (100 - WINDOW_MARGIN_PCT) / 100)
#define WINDOW_DURATION_US    (2 * THEORETICAL_APOGEE_US * WINDOW_MARGIN_PCT / 100)

#include "platform.h"
#include "debug.h"
#include "buzzer.h"
#include "system.h"
#include "drv8872.h"
#include "log.h"

// Drapeau pour le retrait/insertion du commutateur RBF (Remove Before Flight)
extern volatile uint8_t triggerRBF;

/**
 * @brief Initialise le séquenceur de vol, configure les interruptions de sécurité et lit l'état initial des GPIOs.
 * @return L'état de départ du vol (PRE_FLIGHT).
 */
rocket_state_t seq_init(void);

/**
 * @brief Gère les transitions de la machine d'état principale de vol.
 * @return L'état actuel mis à jour.
 */
rocket_state_t seq_handle(void);

/**
 * @brief Enregistre un événement de vol dans les logs de la flash.
 */
void seq_log_event(const char* evt);

// --- Fonctions de gestion individuelle des phases de vol ---

rocket_state_t seq_preLaunch(void);
rocket_state_t seq_pyroRdy(void);
rocket_state_t seq_ascend(void);
rocket_state_t seq_window(void);
rocket_state_t seq_deploy(void);
rocket_state_t seq_descend(void);
rocket_state_t seq_touchdown(void);

// --- Fonctions d'interruptions matérielles (ISR Callbacks) ---

/**
 * @brief Callback d'interruption générale GPIO (RP2040) pour le Jack, le RBF et les capteurs Octo.
 */
void seq_gpio_callback(uint gpio, uint32_t events);

/**
 * @brief Callback d'interruption déclenchée à chaque nouvelle donnée barométrique prête (25 Hz).
 */
void seq_baro_drdy_callback(void);

/**
 * @brief Callback d'alarme déclenchée pour ouvrir la fenêtre d'ouverture du parachute.
 */
int64_t seq_is_window_open_callback(alarm_id_t id, void* user_data);

/**
 * @brief Callback d'alarme déclenchée à l'expiration de la fenêtre d'ouverture (déploiement de secours).
 */
int64_t seq_window_timeout_callback(alarm_id_t id, void* user_data);

/**
 * @brief Callback d'alarme déclenchée pour signaler la fin estimée de la descente (Touchdown).
 */
int64_t seq_touchdown_timeout_callback(alarm_id_t id, void* user_data);

void seq_start_window_timer();
void seq_reset_triggers();

void seq_detect_apogee(void);
void seq_deploy_recovery(void);

#endif // SEQUENCER_HEADER_FILE 