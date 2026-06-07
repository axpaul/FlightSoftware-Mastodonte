// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : lsm6dsl.h
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Custom low-level driver for ST LSM6DSL IMU (Accel + Gyro)
// -------------------------------------------------------------

#ifndef LSM6DSL_HEADER_FILE
#define LSM6DSL_HEADER_FILE

#include "pico/stdlib.h"

#define LSM6DSL_I2C_ADDR  0x6A  // Adresse I2C par défaut du LSM6DSL (SDO = 0)

// Registres du LSM6DSL
#define LSM_WHO_AM_I      0x0F  // Registre d'identification (doit renvoyer 0x6A)
#define LSM_CTRL1_XL      0x10  // Contrôle accéléromètre (ODR, échelle)
#define LSM_CTRL2_G       0x11  // Contrôle gyroscope (ODR, échelle)
#define LSM_CTRL3_C       0x12  // Contrôle 3 (auto-increment...)
#define LSM_OUTX_L_G      0x22  // Gyroscope X LSB
#define LSM_OUTX_L_XL     0x28  // Accéléromètre X LSB

/**
 * @brief Initialise le capteur LSM6DSL, vérifie sa présence sur le bus, et configure l'accéléromètre et le gyroscope.
 * @return true si le capteur est détecté et configuré, false sinon.
 */
bool lsm6dsl_init(void);

/**
 * @brief Lit l'accélération sur les 3 axes.
 * @param ax Pointeur vers le float qui recevra l'accélération X en g.
 * @param ay Pointeur vers le float qui recevra l'accélération Y en g.
 * @param az Pointeur vers le float qui recevra l'accélération Z en g.
 */
void lsm6dsl_read_accel(float* ax, float* ay, float* az);

/**
 * @brief Lit les vitesses angulaires sur les 3 axes.
 * @param gx Pointeur vers le float qui recevra la vitesse angulaire X en dps.
 * @param gy Pointeur vers le float qui recevra la vitesse angulaire Y en dps.
 * @param gz Pointeur vers le float qui recevra la vitesse angulaire Z en dps.
 */
void lsm6dsl_read_gyro(float* gx, float* gy, float* gz);

#endif // LSM6DSL_HEADER_FILE
