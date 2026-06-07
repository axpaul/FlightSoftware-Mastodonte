// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : telemetry.cpp
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Module de suivi des statistiques de vol et du rapport final
// -------------------------------------------------------------

#include <Arduino.h>
#include "telemetry.h"
#include "lps22hb.h"
#include "lsm6dsl.h"
#include "sequencer.h"
#include "log.h"
#include "debug.h"
#include <math.h>

static float max_vel = 0.0f;
static float max_accel = 0.0f;
static float apogee_alt = 0.0f;
static uint32_t apogee_time_ms = 0;
static uint32_t t_launch = 0;
static bool apogee_recorded = false;

void telemetry_init(void) {
    max_vel = 0.0f;
    max_accel = 0.0f;
    apogee_alt = 0.0f;
    apogee_time_ms = 0;
    t_launch = millis();
    apogee_recorded = false;
    log_entry("[TELEMETRY] Init done, launch time recorded.");
}

void telemetry_update(void) {
    // 1. Suivi de l'accélération maximale (magnitude vectorielle)
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    lsm6dsl_read_accel(&ax, &ay, &az);
    float acc_mag = sqrtf(ax*ax + ay*ay + az*az);
    if (acc_mag > max_accel) {
        max_accel = acc_mag;
    }

    // 2. Suivi de la vitesse maximale verticale (Kalman)
    float v = lps22hb_get_kalman_velocity();
    if (v > max_vel) {
        max_vel = v;
    }

    // 3. Suivi et capture des conditions à l'apogée
    if (triggerBaroApogee == 1 && !apogee_recorded) {
        apogee_alt = lps22hb_get_kalman_altitude();
        apogee_time_ms = millis() - t_launch;
        apogee_recorded = true;
    }
}

void telemetry_save_report(void) {
    float max_alt = lps22hb_get_max_altitude();

    log_entry("[REPORT] ==================================");
    log_entry("[REPORT]        RAPPORT DE VOL FINAL       ");
    log_entry("[REPORT] ==================================");
    log_entryf("[REPORT] Version Firmware : %s", FW_VERSION);
    log_entryf("[REPORT] Altitude Max     : %.2f m", max_alt);
    log_entryf("[REPORT] Vitesse Max      : %.2f m/s", max_vel);
    log_entryf("[REPORT] Accel Max        : %.2f g", max_accel);
    if (apogee_recorded) {
        log_entryf("[REPORT] Apogee Est.      : %.2f m a T+%.2f s", apogee_alt, (float)apogee_time_ms / 1000.0f);
    } else {
        log_entry("[REPORT] Apogee Est.      : Non detectee par le filtre");
    }
    log_entry("[REPORT] ==================================");

    // Affichage direct sur le port série également pour le debug temps réel
    debug_println("\n==================================");
    debug_println("        RAPPORT DE VOL FINAL       ");
    debug_println("==================================");
    debug_printf("[REPORT] Version Firmware : %s\n", FW_VERSION);
    debug_printf("[REPORT] Altitude Max     : %.2f m\n", max_alt);
    debug_printf("[REPORT] Vitesse Max      : %.2f m/s\n", max_vel);
    debug_printf("[REPORT] Accel Max        : %.2f g\n", max_accel);
    if (apogee_recorded) {
        debug_printf("[REPORT] Apogee Est.      : %.2f m a T+%.2f s\n", apogee_alt, (float)apogee_time_ms / 1000.0f);
    } else {
        debug_println("[REPORT] Apogee Est.      : Non detectee par le filtre");
    }
    debug_println("==================================\n");
}
