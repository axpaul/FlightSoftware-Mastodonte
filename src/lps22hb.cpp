// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : lps22hb.cpp
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Custom low-level driver for ST LPS22HB barometer
// -------------------------------------------------------------

#include "lps22hb.h"
#include "lsm6dsl.h"
#include "hardware/i2c.h"
#include "log.h"
#include "debug.h"
#include <math.h>
#include "sequencer.h"
#include "telemetry.h"

bool baro_present = false;

// Variables d'estimation et de filtrage (Kalman 1D)
static float ground_pressure = -1.0f;
static float kalman_z = 0.0f;
static float kalman_v = 0.0f;
static float P_cov[2][2] = {{1.0f, 0.0f}, {0.0f, 1.0f}};
static float max_altitude = 0.0f;
static uint32_t apogee_counter = 0;
static uint32_t touchdown_confirm_counter = 0;
static volatile float last_pressure_cached = 1013.25f;

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
    // DRDY = 1 (bit 2 = 1) -> Active le signal Data-ready sur la broche d'interruption INT
    lps_write_reg(0x12, 0x04);
    
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
    float press = (float)raw_press / 4096.0f;
    last_pressure_cached = press;
    return press;
}

float lps22hb_get_last_pressure(void) {
    return last_pressure_cached;
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
    apogee_counter = 0;
}

void lps22hb_update_kalman(void) {
    // Évite de faire tourner le filtre si le baromètre est absent ou non calibré au sol
    if (!baro_present || ground_pressure < 0.0f) return;

    // 1. Lecture de la pression brute et conversion en altitude brute relative (mètres)
    float press = lps22hb_read_pressure();
    float z_meas = lps22hb_hpa_to_altitude(press, ground_pressure);

    // 2. Étape de PREDICTION de l'état (physique de Newton : z = z + v*dt, v = v)
    // dt = 40 ms (fréquence d'échantillonnage de 25 Hz)
    float dt = 0.040f;
    float z_pred = kalman_z + (kalman_v * dt);
    float v_pred = kalman_v;

    // 3. Étape de PREDICTION de la covariance de l'erreur (P = A*P*A^T + Q)
    // On propage l'incertitude sur l'altitude et la vitesse, augmentée des bruits de process Q
    P_cov[0][0] = P_cov[0][0] + dt * (P_cov[1][0] + P_cov[0][1]) + dt * dt * P_cov[1][1] + Q_alt;
    P_cov[0][1] = P_cov[0][1] + dt * P_cov[1][1];
    P_cov[1][0] = P_cov[0][1];
    P_cov[1][1] = P_cov[1][1] + Q_vel;

    // 4. Étape de CORRECTION : Calcul du Gain de Kalman (K)
    // S représente la variance de l'innovation (variance de l'erreur + bruit de mesure R)
    float S = P_cov[0][0] + R_alt;
    // K0 (gain d'altitude) et K1 (gain de vitesse) dosent la confiance accordée entre
    // notre modèle physique de prédiction théorique et la mesure brute bruitée du capteur
    float K0 = P_cov[0][0] / S;
    float K1 = P_cov[1][0] / S;

    // 5. Calcul de l'innovation (résidu : y = mesure - prédiction)
    float y = z_meas - z_pred;

    // 6. Mise à jour de l'état estimé avec la mesure pondérée par le gain de Kalman
    kalman_z = z_pred + (K0 * y);
    kalman_v = v_pred + (K1 * y);

    // Enregistrement de l'altitude maximale filtrée atteinte
    if (kalman_z > max_altitude) {
        max_altitude = kalman_z;
    }

    // 7. Mise à jour de la covariance de l'erreur pour le prochain cycle (P = (I - K*H)*P)
    // Réduit l'incertitude de l'estimation après avoir pris en compte la nouvelle mesure
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

void lps22hb_drdy_callback(void) {
    if (!baro_present) {
        return; // Bypass de sécurité si le baromètre est physiquement absent
    }

    static bool first_interrupt = false;
    if (!first_interrupt) {
        first_interrupt = true;
        debug_println("[BARO] First DRDY interrupt received on GP5!");
        log_entry("[BARO] First DRDY interrupt received");
    }

    // Fait clignoter la LED verte GP25 pour confirmer l'acquisition active
    gpio_put(PIN_LED_STATUS, !gpio_get(PIN_LED_STATUS));

    // 1. Si on est au sol, on calibre la pression de référence
    if (currentState == PRE_FLIGHT) {
        lps22hb_calibrate_ground();
        // Mettre à jour l'accéléromètre pour l'affichage au sol (sans bloquer)
        float ax, ay, az;
        lsm6dsl_read_accel(&ax, &ay, &az);
        return;
    }

    // 2. Si on est en vol, on met à jour l'estimateur de Kalman et la télémétrie
    if (currentState == ASCEND || currentState == WINDOW || currentState == DESCEND) {
        lps22hb_update_kalman();
        telemetry_update();
    }
    // 3. Dans les autres états (ex: PYRO_RDY, TOUCHDOWN, etc.), on doit quand même lire la pression
    // pour acquitter l'interruption matérielle du capteur, sinon son pin INT reste bloqué.
    else {
        (void)lps22hb_read_pressure();
        // Mettre à jour l'accéléromètre pour l'affichage
        float ax, ay, az;
        lsm6dsl_read_accel(&ax, &ay, &az);
    }

    // 3. Algorithme de détection d'apogée (uniquement en montée ou dans la fenêtre)
    if (currentState == ASCEND || currentState == WINDOW) {
        float z = lps22hb_get_kalman_altitude();
        float v = lps22hb_get_kalman_velocity();

        // Condition : altitude > 15m (marge de sécurité) et vitesse estimée négative (redescente)
        if (z > 15.0f && v < -1.0f) {
            apogee_counter++;
            if (apogee_counter >= 5) { // 5 mesures consécutives (~200 ms) pour confirmer
                if (triggerBaroApogee == 0) {
                    triggerBaroApogee = 1;
                    log_entryf("[KALMAN] Apogee detectee ! Alt = %.2fm, Vel = %.2fm/s", z, v);
                    debug_printf("[KALMAN] Apogee detectee ! Alt = %.2f m, Vel = %.2f m/s\n", z, v);
                }
            }
        } else {
            apogee_counter = 0; // Réinitialise en cas de remontée ou d'altitude insuffisante
        }
    }

    // 4. Détection dynamique de l'atterrissage (Touchdown) par vitesse de Kalman nulle
    if (currentState == DESCEND) {
        float v = lps22hb_get_kalman_velocity();
        if (fabsf(v) < 0.5f) {
            touchdown_confirm_counter++;
            if (touchdown_confirm_counter >= 50) { // 50 ticks * 40ms = 2 secondes à 25Hz
                if (triggerTouch == 0) {
                    triggerTouch = 1;
                    log_entryf("[KALMAN] Touchdown detecte par capteur (vitesse stabilisee a %.2f m/s)", v);
                    debug_printf("[KALMAN] Touchdown detecte par capteur (vitesse stabilisee a %.2f m/s)\n", v);
                }
            }
        } else {
            touchdown_confirm_counter = 0; // Réinitialise si la vitesse varie
        }
    }
}
