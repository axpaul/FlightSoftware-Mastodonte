// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : drv8872.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Driver DRV8872 (contrôle moteur H-bridge)
// -------------------------------------------------------------

#ifndef DRV8872_H
#define DRV8872_H

#include <stdlib.h>              // Pour malloc() et free()
#include "pico/stdlib.h"
#include "hardware/pwm.h"        // Accès au PWM hardware Pico SDK

#include "debug.h"
#include "platform.h"            // Définitions des broches INx_Mx, NFAUT_Mx
#include "log.h"

// === Structures ===

// Structure individuelle de moteur
typedef struct {
    uint in1_pin;
    uint in2_pin;
    uint fault_pin;
    void (*fault_callback)(void);
} drv8872_t;

// Structure de groupe de moteurs
typedef struct {
    drv8872_t* motors[3];   // Jusqu'à 3 moteurs
    uint8_t motor_count;    // Nombre de moteurs actifs (1 à 3)
    bool direction;         // true = forward, false = reverse
} drv8872_group_t;

// === Instances accessibles globalement ===
extern drv8872_t motor1;
extern drv8872_t motor2;
extern drv8872_t motor3;
extern drv8872_t* motors[3];
extern drv8872_group_t group_all_motors;

// === Initialisation ===
void drv8872_init(drv8872_t* drv);
drv8872_t* drv8872_get(uint8_t index);  // Accès indexé

// === Commandes simples ===
void drv8872_forward(drv8872_t* drv);
void drv8872_reverse(drv8872_t* drv);
void drv8872_brake(drv8872_t* drv);
void drv8872_stop(drv8872_t* drv);

// === Commandes PWM ===
void drv8872_forward_pwm(drv8872_t* drv, uint slice_num, uint level);
void drv8872_reverse_pwm(drv8872_t* drv, uint slice_num, uint level);

// === Diagnostics ===
// Ne pas utiliser dans le projet, conflit avec sequencer
bool drv8872_is_fault(drv8872_t* drv);
void drv8872_setup_fault_interrupt(drv8872_t* drv);               // Callback utilisateur (1 moteur)
// void drv8872_setup_raw_fault_monitors(void);                      // IRQ brutes globales (debug)
// void drv8872_fault_raw_callback(uint gpio, uint32_t events);      // Handler IRQ

// === Contrôle de groupe avec minuterie ===
void drv8872_group_activate_for_us(drv8872_group_t* group, uint64_t duration_us);

// === Fonctions de test et de debug ===
void motor_diag_init(void);
void motor_diag_test_sequence(void);
void motor_diag_log_faults(void);

#endif // DRV8872_H
