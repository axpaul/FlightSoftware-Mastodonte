// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : drv8872.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Driver DRV8872 (contrôle moteur H-bridge)
// -------------------------------------------------------------

#include "drv8872.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "debug.h"
#include <stdlib.h>
#include "system.h"

// === Callbacks d'interruption en cas de défaut sur les moteurs ===
static void motor1_fault_handler(void) {
    gpio_set_irq_enabled(NFAUT_M1, GPIO_IRQ_EDGE_FALL, false);
    drv8872_stop(&motor1);
    log_entry("[DRV] DEFAUT : Probleme detecte sur le Moteur 1 ! Arret de securite.");
    debug_println("[DRV] DEFAUT : Probleme detecte sur le Moteur 1 ! Arret de securite.");
    if (currentState == PRE_FLIGHT || currentState == PYRO_RDY) {
        currentState = ERROR_MOTOR;
        apply_state_config(ERROR_MOTOR);
    }
}
static void motor2_fault_handler(void) {
    gpio_set_irq_enabled(NFAUT_M2, GPIO_IRQ_EDGE_FALL, false);
    drv8872_stop(&motor2);
    log_entry("[DRV] DEFAUT : Probleme detecte sur le Moteur 2 ! Arret de securite.");
    debug_println("[DRV] DEFAUT : Probleme detecte sur le Moteur 2 ! Arret de securite.");
    if (currentState == PRE_FLIGHT || currentState == PYRO_RDY) {
        currentState = ERROR_MOTOR;
        apply_state_config(ERROR_MOTOR);
    }
}
static void motor3_fault_handler(void) {
    gpio_set_irq_enabled(NFAUT_M3, GPIO_IRQ_EDGE_FALL, false);
    drv8872_stop(&motor3);
    log_entry("[DRV] DEFAUT : Probleme detecte sur le Moteur 3 ! Arret de securite.");
    debug_println("[DRV] DEFAUT : Probleme detecte sur le Moteur 3 ! Arret de securite.");
    if (currentState == PRE_FLIGHT || currentState == PYRO_RDY) {
        currentState = ERROR_MOTOR;
        apply_state_config(ERROR_MOTOR);
    }
}

// === Définition de 3 instances de moteurs globalement accessibles ===
drv8872_t motor1 = { IN1_M1, IN2_M1, NFAUT_M1, motor1_fault_handler };
drv8872_t motor2 = { IN1_M2, IN2_M2, NFAUT_M2, motor2_fault_handler };
drv8872_t motor3 = { IN1_M3, IN2_M3, NFAUT_M3, motor3_fault_handler };
drv8872_t* motors[3] = { &motor1, &motor2, &motor3 };

drv8872_group_t group_all_motors = {
    .motors = { &motor1, &motor2, &motor3 },
    .motor_count = 3,
    .direction = true
};

// === Structure pour alarmes temporisées de groupe ===
typedef struct {
    drv8872_group_t group;
} drv_group_timer_context_t;

// === Aiguillage du défaut moteur selon le GPIO qui a déclenché l'interruption ===
void drv8872_handle_fault(uint gpio) {
    for (int i = 0; i < 3; i++) {
        if (motors[i] && motors[i]->fault_pin == gpio) {
            if (motors[i]->fault_callback) {
                motors[i]->fault_callback();
            }
            break;
        }
    }
}

// === Initialisation de base ===
void drv8872_init(drv8872_t* drv) {
    log_entryf("[DRV] Init DRV8872 on IN1=%d IN2=%d", drv->in1_pin, drv->in2_pin);
    drv8872_stop(drv);
}

// === Accès via index ===
drv8872_t* drv8872_get(uint8_t index) {
    if (index >= 3) return NULL;
    return motors[index];
}

// === Contrôles basiques ===
void drv8872_forward(drv8872_t* drv) {
    log_entryf("[DRV] Forward motor (IN1=%d, IN2=%d)", drv->in1_pin, drv->in2_pin);
    gpio_init(drv->in1_pin);
    gpio_init(drv->in2_pin);
    gpio_set_dir(drv->in1_pin, GPIO_OUT);
    gpio_set_dir(drv->in2_pin, GPIO_OUT);
    gpio_put(drv->in1_pin, 1);
    gpio_put(drv->in2_pin, 0);
}

void drv8872_reverse(drv8872_t* drv) {
    log_entryf("[DRV] Reverse motor (IN1=%d, IN2=%d)", drv->in1_pin, drv->in2_pin);
    gpio_init(drv->in1_pin);
    gpio_init(drv->in2_pin);
    gpio_set_dir(drv->in1_pin, GPIO_OUT);
    gpio_set_dir(drv->in2_pin, GPIO_OUT);
    gpio_put(drv->in1_pin, 0);
    gpio_put(drv->in2_pin, 1);
}

void drv8872_brake(drv8872_t* drv) {
    log_entryf("[DRV] Brake motor (IN1=%d, IN2=%d)", drv->in1_pin, drv->in2_pin);
    gpio_init(drv->in1_pin);
    gpio_init(drv->in2_pin);
    gpio_set_dir(drv->in1_pin, GPIO_OUT);
    gpio_set_dir(drv->in2_pin, GPIO_OUT);
    gpio_put(drv->in1_pin, 1);
    gpio_put(drv->in2_pin, 1);
}

void drv8872_stop(drv8872_t* drv) {
    log_entryf("[DRV] Stop motor (IN1=%d, IN2=%d)", drv->in1_pin, drv->in2_pin);
    gpio_init(drv->in1_pin);
    gpio_init(drv->in2_pin);
    gpio_set_dir(drv->in1_pin, GPIO_OUT);
    gpio_set_dir(drv->in2_pin, GPIO_OUT);
    gpio_put(drv->in1_pin, 0);
    gpio_put(drv->in2_pin, 0);
}

// === Contrôles PWM ===
void drv8872_forward_pwm(drv8872_t* drv, uint slice_num, uint level) {
    log_entryf("[DRV][PWM] Forward PWM motor on pin %d - level %d", drv->in1_pin, level);
    gpio_set_function(drv->in1_pin, GPIO_FUNC_PWM);
    gpio_put(drv->in2_pin, 0);
    pwm_set_gpio_level(drv->in1_pin, level);
    pwm_set_enabled(slice_num, true);
}

void drv8872_reverse_pwm(drv8872_t* drv, uint slice_num, uint level) {
    log_entryf("[DRV][PWM] Reverse PWM motor on pin %d - level %d", drv->in2_pin, level);
    gpio_set_function(drv->in2_pin, GPIO_FUNC_PWM);
    gpio_put(drv->in1_pin, 0);
    pwm_set_gpio_level(drv->in2_pin, level);
    pwm_set_enabled(slice_num, true);
}

// === Diagnostic ===
bool drv8872_is_fault(drv8872_t* drv) {
    return gpio_get(drv->fault_pin) == 0;
}

void drv8872_setup_fault_interrupt(drv8872_t* drv) {
    log_entryf("[DRV] Fault interrupt set up on pin %d", drv->fault_pin);
    if (!drv->fault_callback) return;
    gpio_set_irq_enabled(drv->fault_pin, GPIO_IRQ_EDGE_FALL, true);
}

// === Activation groupe avec timer ===
static int64_t drv8872_group_alarm_callback(alarm_id_t id, void* user_data) {
    log_entry("[DRV_GROUP] stopping all motors");
    drv_group_timer_context_t* ctx = (drv_group_timer_context_t*)user_data;
    if (ctx) {
        for (uint8_t i = 0; i < ctx->group.motor_count; i++) {
            if (ctx->group.motors[i]) {
                drv8872_stop(ctx->group.motors[i]);
            }
        }
        free(ctx);
   
        // 🔍 Log d'état des moteurs après arrêt automatique
        debug_println("[DRV8872] Motors stopped by timer callback. Checking faults...");
        motor_diag_log_faults();
    }
    return 0;
}

void drv8872_group_activate_for_us(drv8872_group_t* group, uint64_t duration_us) {

    if (!group || group->motor_count == 0) return;

    for (uint8_t i = 0; i < group->motor_count; i++) {
        if (group->motors[i]) {
            log_entryf("[DRV_GROUP] Motor[%d]", i);
            if (group->direction) drv8872_forward(group->motors[i]);
            else drv8872_reverse(group->motors[i]);
        }
    }

    drv_group_timer_context_t* ctx = (drv_group_timer_context_t*) malloc(sizeof(drv_group_timer_context_t));
    if (!ctx) return;

    ctx->group = *group;
    add_alarm_in_us(duration_us, drv8872_group_alarm_callback, ctx, true);
}

// === Diagnostic complet ===
void motor_diag_init(void) {
    log_entry("[MOTOR_DIAG] Logging motor fault states");
    drv8872_init(&motor1);
    drv8872_init(&motor2);
    drv8872_init(&motor3);
}

void motor_diag_log_faults(void) {
    bool fault1 = drv8872_is_fault(&motor1);
    bool fault2 = drv8872_is_fault(&motor2);
    bool fault3 = drv8872_is_fault(&motor3);  

    debug_printf("[MOTOR_DIAG] Fault M1: %s\n", fault1 ? "YES" : "NO");
    debug_printf("[MOTOR_DIAG] Fault M2: %s\n", fault2 ? "YES" : "NO");
    debug_printf("[MOTOR_DIAG] Fault M3: %s\n", fault3 ? "YES" : "NO");

    log_entryf("[MOTOR_DIAG] Fault M1: %s", fault1 ? "YES" : "NO");
    log_entryf("[MOTOR_DIAG] Fault M2: %s", fault2 ? "YES" : "NO");
    log_entryf("[MOTOR_DIAG] Fault M3: %s", fault3 ? "YES" : "NO");
}

void motor_diag_test_sequence(void) {
    debug_println("[MOTOR_DIAG] Start test sequence");

    drv8872_forward(&motor1); drv8872_forward(&motor2); drv8872_forward(&motor3);
    debug_println("[MOTOR_DIAG] Forward"); sleep_ms(5000); motor_diag_log_faults();

    drv8872_reverse(&motor1); drv8872_reverse(&motor2); drv8872_reverse(&motor3);
    debug_println("[MOTOR_DIAG] Reverse"); sleep_ms(5000); motor_diag_log_faults();

    drv8872_brake(&motor1); drv8872_brake(&motor2); drv8872_brake(&motor3);
    debug_println("[MOTOR_DIAG] Brake"); sleep_ms(1000); motor_diag_log_faults();

    drv8872_stop(&motor1); drv8872_stop(&motor2); drv8872_stop(&motor3);
    debug_println("[MOTOR_DIAG] Stop"); sleep_ms(1000); motor_diag_log_faults();

    debug_println("[MOTOR_DIAG] Test complete");
}

bool drv8872_test_short_circuit(drv8872_t* drv) {
    if (!drv) return false;

    // 1. Désactive temporairement l'interruption sur ce pin nFAULT pour éviter d'invoquer l'ISR de défaut
    gpio_set_irq_enabled(drv->fault_pin, GPIO_IRQ_EDGE_FALL, false);

    // 2. Pulse brièvement le moteur (IN1=1, IN2=0) en marche avant
    gpio_put(drv->in1_pin, 1);
    gpio_put(drv->in2_pin, 0);

    // 3. Attend 2 millisecondes pour laisser le temps au DRV8872 de réagir et de lever le défaut
    sleep_ms(2);

    // 4. Lit le niveau logique de nFAULT. S'il est LOW, c'est qu'un court-circuit ou une surcharge est actif.
    bool has_fault = (gpio_get(drv->fault_pin) == 0);

    // 5. Arrête le moteur immédiatement (IN1=0, IN2=0)
    gpio_put(drv->in1_pin, 0);
    gpio_put(drv->in2_pin, 0);

    // Attente supplémentaire de sécurité pour s'assurer que la puce dissipe l'énergie et que nFAULT remonte
    sleep_ms(1);

    // 6. Réactive l'interruption pour le fonctionnement normal
    gpio_set_irq_enabled(drv->fault_pin, GPIO_IRQ_EDGE_FALL, true);

    return has_fault;
}

bool motor_diag_run_self_test(void) {
    log_entry("[MOTOR_DIAG] Startup self-test initiated");
    bool success = true;

    bool m1 = drv8872_test_short_circuit(&motor1);
    bool m2 = drv8872_test_short_circuit(&motor2);
    bool m3 = drv8872_test_short_circuit(&motor3);

    debug_printf("       - Moteur 1 : %s\n", m1 ? "DEFAUT" : "OK");
    debug_printf("       - Moteur 2 : %s\n", m2 ? "DEFAUT" : "OK");
    debug_printf("       - Moteur 3 : %s\n", m3 ? "DEFAUT" : "OK");

    if (m1) { log_entry("[MOTOR_DIAG] ERROR: Short-circuit on Motor 1"); success = false; }
    if (m2) { log_entry("[MOTOR_DIAG] ERROR: Short-circuit on Motor 2"); success = false; }
    if (m3) { log_entry("[MOTOR_DIAG] ERROR: Short-circuit on Motor 3"); success = false; }

    if (success) {
        log_entry("[MOTOR_DIAG] Self-test passed successfully");
    } else {
        log_entry("[MOTOR_DIAG] Self-test failed");
    }

    return success;
}
