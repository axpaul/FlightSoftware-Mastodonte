// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : lps22hb.cpp
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Custom low-level driver for ST LPS22HB barometer
// -------------------------------------------------------------

#include "lps22hb.h"
#include "hardware/i2c.h"
#include "log.h"
#include "debug.h"
#include <math.h>

bool baro_present = false;

// Variables d'estimation et de filtrage (Kalman 1D)
static float ground_pressure = -1.0f;
static float kalman_z = 0.0f;
static float kalman_v = 0.0f;
static float P_cov[2][2] = {{1.0f, 0.0f}, {0.0f, 1.0f}};
static float max_altitude = 0.0f;

static const float Q_alt = 0.1f;
static const float Q_vel = 0.5f;
static const float R_alt = 1.0f;

// --- Fonctions d'écriture/lecture de bas niveau I2C ---

static void lps_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c1, LPS22HB_I2C_ADDR, buf, 2, false);
}

static uint8_t lps_read_reg(uint8_t reg) {
    uint8_t val = 0;
    i2c_write_blocking(i2c1, LPS22HB_I2C_ADDR, &reg, 1, true); // true = repeated start
    i2c_read_blocking(i2c1, LPS22HB_I2C_ADDR, &val, 1, false);
    return val;
}

static void lps_read_regs(uint8_t start_reg, uint8_t *dst, size_t len) {
    i2c_write_blocking(i2c1, LPS22HB_I2C_ADDR, &start_reg, 1, true); // true = repeated start
    i2c_read_blocking(i2c1, LPS22HB_I2C_ADDR, dst, len, false);
}

// --- API Publique ---

bool lps22hb_init(void) {
    // 1. Lecture du WHO_AM_I pour vérifier la présence du capteur sur le bus
    uint8_t who = lps_read_reg(LPS_WHO_AM_I);
    if (who != 0xB1) {
        log_entryf("[LPS22HB] ERROR: Identifiant incorrect (WHO_AM_I = 0x%02X, attendu 0xB1)", who);
        baro_present = false;
        return false;
    }

    baro_present = true;
    ground_pressure = -1.0f;
    lps22hb_reset_kalman();
    log_entry("[LPS22HB] Capteur detecte avec succes.");

    // 2. Configuration : CTRL_REG1 (0x10)
    // ODR = 25 Hz (bits 6:4 = 011), BDU (Block Data Update) = 1 (bit 1 = 1) -> 0x32
    // BDU évite de lire des valeurs incohérentes entre les parties hautes et basses des registres.
    lps_write_reg(LPS_CTRL_REG1, 0x32);

    // 3. Configuration de l'interruption : CTRL_REG3 (0x12)
    // DRDY = 1 (bit 3 = 1) -> Active le signal Data-ready sur la broche d'interruption INT
    lps_write_reg(0x12, 0x08);
    
    log_entry("[LPS22HB] Initialisation reussie (Mode 25 Hz, Interrupt DRDY active).");
    return true;
}

float lps22hb_read_pressure(void) {
    uint8_t buf[3];
    lps_read_regs(LPS_PRESS_OUT_XL, buf, 3);

    // Reconstruction de la valeur sur 24 bits
    int32_t raw_press = buf[0] | (buf[1] << 8) | (buf[2] << 16);

    // Extension de signe si la valeur est négative (complément à deux sur 24 bits)
    if (raw_press & 0x00800000) {
        raw_press |= 0xFF000000;
    }

    // Conversion en hPa (1 LSB = 1/4096 hPa)
    return (float)raw_press / 4096.0f;
}

float lps22hb_read_temperature(void) {
    uint8_t buf[2];
    lps_read_regs(LPS_TEMP_OUT_L, buf, 2);

    // Reconstruction de la valeur sur 16 bits signés
    int16_t raw_temp = buf[0] | (buf[1] << 8);

    // Conversion en °C (1 LSB = 1/100 °C)
    return (float)raw_temp / 100.0f;
}

float lps22hb_hpa_to_altitude(float pressure_hpa, float ground_pressure_hpa) {
    if (ground_pressure_hpa <= 0.0f) return 0.0f;
    // Formule barométrique internationale
    return 44330.0f * (1.0f - powf(pressure_hpa / ground_pressure_hpa, 0.1902949f));
}

void lps22hb_calibrate_ground(void) {
    if (!baro_present) return;
    float press = lps22hb_read_pressure();
    if (ground_pressure < 0.0f) {
        ground_pressure = press;
    } else {
        ground_pressure = (ground_pressure * 0.98f) + (press * 0.02f);
    }
}

void lps22hb_reset_kalman(void) {
    kalman_z = 0.0f;
    kalman_v = 0.0f;
    P_cov[0][0] = 1.0f; P_cov[0][1] = 0.0f;
    P_cov[1][0] = 0.0f; P_cov[1][1] = 1.0f;
    max_altitude = 0.0f;
}

void lps22hb_update_kalman(void) {
    if (!baro_present || ground_pressure < 0.0f) return;

    float press = lps22hb_read_pressure();
    float z_meas = lps22hb_hpa_to_altitude(press, ground_pressure);

    // Étape de prédiction (dt = 40 ms)
    float dt = 0.040f;
    float z_pred = kalman_z + (kalman_v * dt);
    float v_pred = kalman_v;

    P_cov[0][0] = P_cov[0][0] + dt * (P_cov[1][0] + P_cov[0][1]) + dt * dt * P_cov[1][1] + Q_alt;
    P_cov[0][1] = P_cov[0][1] + dt * P_cov[1][1];
    P_cov[1][0] = P_cov[0][1];
    P_cov[1][1] = P_cov[1][1] + Q_vel;

    // Étape de correction/mise à jour
    float S = P_cov[0][0] + R_alt;
    float K0 = P_cov[0][0] / S;
    float K1 = P_cov[1][0] / S;

    float y = z_meas - z_pred; // Résidu d'innovation

    kalman_z = z_pred + (K0 * y);
    kalman_v = v_pred + (K1 * y);

    if (kalman_z > max_altitude) {
        max_altitude = kalman_z;
    }

    P_cov[0][0] = (1.0f - K0) * P_cov[0][0];
    P_cov[0][1] = (1.0f - K0) * P_cov[0][1];
    P_cov[1][0] = P_cov[0][1];
    P_cov[1][1] = P_cov[1][1] - K1 * P_cov[0][1];
}

float lps22hb_get_kalman_altitude(void) {
    return kalman_z;
}

float lps22hb_get_kalman_velocity(void) {
    return kalman_v;
}

float lps22hb_get_max_altitude(void) {
    return max_altitude;
}

float lps22hb_get_ground_pressure(void) {
    return ground_pressure;
}
