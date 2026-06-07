// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : lps22hb.h
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Custom low-level driver for ST LPS22HB barometer
// -------------------------------------------------------------

#ifndef LPS22HB_HEADER_FILE
#define LPS22HB_HEADER_FILE

#include "pico/stdlib.h"

#define LPS22HB_I2C_ADDR  0x5C  // Adresse I2C par défaut du LPS22HB sur la carte Berry

// Registres du LPS22HB
#define LPS_WHO_AM_I      0x0F  // Registre d'identification (doit renvoyer 0xB1)
#define LPS_CTRL_REG1     0x10  // Registre de contrôle 1 (ODR, filtrage...)
#define LPS_PRESS_OUT_XL  0x28  // Pression en hPa (LSB, 24-bit)
#define LPS_PRESS_OUT_L   0x29  // Pression en hPa (Mid, 24-bit)
#define LPS_PRESS_OUT_H   0x2A  // Pression en hPa (MSB, 24-bit)
#define LPS_TEMP_OUT_L    0x2B  // Température en °C (LSB, 16-bit)
#define LPS_TEMP_OUT_H    0x2C  // Température en °C (MSB, 16-bit)

extern bool baro_present;

/**
 * @brief Initialise le capteur LPS22HB, vérifie sa présence sur le bus, et le configure.
 * @return true si le capteur est détecté et configuré, false sinon.
 */
bool lps22hb_init(void);

/**
 * @brief Lit la pression actuelle en hPa.
 * @return La pression mesurée en hPa.
 */
float lps22hb_read_pressure(void);

/**
 * @brief Lit la température actuelle du capteur en °C.
 * @return La température mesurée en °C.
 */
float lps22hb_read_temperature(void);

/**
 * @brief Convertit la pression mesurée (hPa) en altitude relative (m) par rapport au sol.
 * @param pressure_hpa Pression actuelle.
 * @param ground_pressure_hpa Pression moyenne de référence mesurée au sol.
 * @return L'altitude relative estimée.
 */
float lps22hb_hpa_to_altitude(float pressure_hpa, float ground_pressure_hpa);

/**
 * @brief Exécute la calibration de la pression de référence au sol (PRE_FLIGHT).
 */
void lps22hb_calibrate_ground(void);

/**
 * @brief Réinitialise les variables du filtre de Kalman 1D.
 */
void lps22hb_reset_kalman(void);

/**
 * @brief Met à jour le filtre de Kalman 1D avec la nouvelle mesure du capteur.
 */
void lps22hb_update_kalman(void);

/**
 * @brief Obtient l'altitude estimée par le filtre de Kalman (m).
 */
float lps22hb_get_kalman_altitude(void);

/**
 * @brief Obtient la vitesse verticale estimée par le filtre de Kalman (m/s).
 */
float lps22hb_get_kalman_velocity(void);

/**
 * @brief Obtient l'altitude maximale estimée atteinte (m).
 */
float lps22hb_get_max_altitude(void);

/**
 * @brief Obtient la pression moyenne au sol calibrée (hPa).
 */
float lps22hb_get_ground_pressure(void);

#endif // LPS22HB_HEADER_FILE
