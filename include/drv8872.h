#ifndef DRV8872_H
#define DRV8872_H

#include "pico/stdlib.h"

// Structure de configuration du driver
typedef struct {
    uint in1_pin;
    uint in2_pin;
    uint fault_pin; // Nouveau : nFAULT pin
    void (*fault_callback)(void); // Callback utilisateur
} drv8872_t;

// Initialisation
void drv8872_init(drv8872_t* drv);

// Contrôle basique
void drv8872_forward(drv8872_t* drv);
void drv8872_reverse(drv8872_t* drv);
void drv8872_brake(drv8872_t* drv);
void drv8872_stop(drv8872_t* drv);

// PWM
void drv8872_forward_pwm(drv8872_t* drv, uint slice_num, uint level);
void drv8872_reverse_pwm(drv8872_t* drv, uint slice_num, uint level);

// Diagnostic
bool drv8872_is_fault(drv8872_t* drv);
void drv8872_setup_fault_interrupt(drv8872_t* drv);

#endif
