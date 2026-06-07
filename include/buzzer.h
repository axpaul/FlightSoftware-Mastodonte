// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : buzzer.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Contrôle du buzzer passif (tonalité ou bips périodiques)
// -------------------------------------------------------------

#ifndef BUZZER_HEADER
#define BUZZER_HEADER

#include <stdint.h>

#include "platform.h"
#include "debug.h"

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/rtc.h"
#include "hardware/sync.h"
#include "hardware/structs/scb.h"

/**
 * @brief Structure de configuration persistante pour le contrôle du buzzer.
 */
struct BuzzerConfig {
    uint16_t freq;      // Fréquence du signal en Hz
    uint16_t duration;  // Durée d'activation du bip en millisecondes
};

/**
 * @brief Initialise le buzzer matériel en configurant le PWM sur la broche dédiée.
 */
void buzzer_init();

/**
 * @brief Configure et active/désactive le signal sonore émis par le buzzer.
 * 
 * @param enable Active le buzzer si true, l'éteint si false.
 * @param beatPeriodMs Période de répétition des bips en millisecondes (0 pour un son continu).
 * @param beatDurationMs Durée d'émission de chaque bip en millisecondes.
 * @param frequency Fréquence de la tonalité en Hz (par défaut 2000 Hz).
 */
void setBuzzer(bool enable, uint16_t beatPeriodMs = 0, uint16_t beatDurationMs = 0, uint16_t frequency = 2000);

#endif // BUZZER_HEADER
