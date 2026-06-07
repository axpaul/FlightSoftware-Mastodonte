// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : lsm6dsl.cpp
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Custom low-level driver for ST LSM6DSL IMU (Accel + Gyro)
// -------------------------------------------------------------

#include "lsm6dsl.h"
#include "hardware/i2c.h"
#include "log.h"
#include "debug.h"

// --- Fonctions d'écriture/lecture de bas niveau I2C ---

static void lsm_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c1, LSM6DSL_I2C_ADDR, buf, 2, false);
}

static uint8_t lsm_read_reg(uint8_t reg) {
    uint8_t val = 0;
    i2c_write_blocking(i2c1, LSM6DSL_I2C_ADDR, &reg, 1, true); // true = repeated start
    i2c_read_blocking(i2c1, LSM6DSL_I2C_ADDR, &val, 1, false);
    return val;
}

static void lsm_read_regs(uint8_t start_reg, uint8_t *dst, size_t len) {
    i2c_write_blocking(i2c1, LSM6DSL_I2C_ADDR, &start_reg, 1, true); // true = repeated start
    i2c_read_blocking(i2c1, LSM6DSL_I2C_ADDR, dst, len, false);
}

// --- API Publique ---

static bool imu_present = false;

bool lsm6dsl_init(void) {
    // 1. Lecture du WHO_AM_I pour vérifier la présence du capteur sur le bus
    uint8_t who = lsm_read_reg(LSM_WHO_AM_I);
    if (who != 0x6A) {
        log_entryf("[LSM6DSL] ERROR: Identifiant incorrect (WHO_AM_I = 0x%02X, attendu 0x6A)", who);
        imu_present = false;
        return false;
    }

    imu_present = true;
    log_entry("[LSM6DSL] Capteur detecte avec succes.");

    // 2. Configuration :
    // CTRL1_XL (0x10) : Accéléromètre à 104 Hz (ODR = 0100), échelle ±16g (FS = 01) -> 0x44
    lsm_write_reg(LSM_CTRL1_XL, 0x44);

    // CTRL2_G (0x11) : Gyroscope à 104 Hz (ODR = 0100), échelle ±2000 dps (FS = 11) -> 0x4C
    lsm_write_reg(LSM_CTRL2_G, 0x4C);

    // CTRL3_C (0x12) : BDU = 1 (bit 6 = 1), IF_ADD_INC = 1 (bit 2 = 1) -> 0x44
    // BDU évite de lire des valeurs incohérentes entre les parties hautes et basses des registres.
    // IF_ADD_INC permet l'auto-incrément automatique de l'adresse des registres lors de lectures multiples.
    lsm_write_reg(LSM_CTRL3_C, 0x44);

    log_entry("[LSM6DSL] Initialisation reussie (Accel & Gyro 104 Hz).");
    return true;
}

void lsm6dsl_read_accel(float* ax, float* ay, float* az) {
    if (!imu_present) {
        *ax = 0.0f;
        *ay = 0.0f;
        *az = 0.0f;
        return;
    }

    uint8_t buf[6];
    lsm_read_regs(LSM_OUTX_L_XL, buf, 6);

    // Reconstruction des valeurs 16 bits signées
    int16_t raw_x = buf[0] | (buf[1] << 8);
    int16_t raw_y = buf[2] | (buf[3] << 8);
    int16_t raw_z = buf[4] | (buf[5] << 8);

    // Conversion en g (sensibilité à ±16g = 0.488 mg/LSB soit 0.488/1000 g/LSB)
    *ax = (float)raw_x * 0.488f / 1000.0f;
    *ay = (float)raw_y * 0.488f / 1000.0f;
    *az = (float)raw_z * 0.488f / 1000.0f;
}

void lsm6dsl_read_gyro(float* gx, float* gy, float* gz) {
    if (!imu_present) {
        *gx = 0.0f;
        *gy = 0.0f;
        *gz = 0.0f;
        return;
    }

    uint8_t buf[6];
    lsm_read_regs(LSM_OUTX_L_G, buf, 6);

    // Reconstruction des valeurs 16 bits signées
    int16_t raw_x = buf[0] | (buf[1] << 8);
    int16_t raw_y = buf[2] | (buf[3] << 8);
    int16_t raw_z = buf[4] | (buf[5] << 8);

    // Conversion en dps (sensibilité à ±2000 dps = 70.0 mdps/LSB soit 70.0/1000 dps/LSB)
    *gx = (float)raw_x * 70.0f / 1000.0f;
    *gy = (float)raw_y * 70.0f / 1000.0f;
    *gz = (float)raw_z * 70.0f / 1000.0f;
}
