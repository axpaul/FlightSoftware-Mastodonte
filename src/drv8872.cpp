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

// === Définition de 3 instances de moteurs globalement accessibles ===
drv8872_t motor1 = { IN1_M1, IN2_M1, NFAUT_M1, NULL };
drv8872_t motor2 = { IN1_M2, IN2_M2, NFAUT_M2, NULL };
drv8872_t motor3 = { IN1_M3, IN2_M3, NFAUT_M3, NULL };
drv8872_t* motors[3] = { &motor1, &motor2, &motor3 };

drv8872_group_t group_all_motors = {
    .motors = { &motor1, &motor2, &motor3 },
    .motor_count = 3,
    .direction = true
};

// === Instance statique alternative (accès isolé par index) ===
static drv8872_t drv_array[3] = {
    { IN1_M1, IN2_M1, NFAUT_M1, NULL },
    { IN1_M2, IN2_M2, NFAUT_M2, NULL },
    { IN1_M3, IN2_M3, NFAUT_M3, NULL },
};

// === Structure pour alarmes temporisées de groupe ===
typedef struct {
    drv8872_group_t group;
} drv_group_timer_context_t;

// === Pour gestion callback simple ===
static drv8872_t* _drv_ref = NULL;

// === Callback fault individuel par GPIO (fallback) ===
static void gpio_fault_callback(uint gpio, uint32_t events) {
    if (_drv_ref && _drv_ref->fault_callback) {
        _drv_ref->fault_callback();
    }
}

// === Initialisation de base ===
void drv8872_init(drv8872_t* drv) {
    drv8872_stop(drv);
    _drv_ref = drv;
}

// === Accès via index ===
drv8872_t* drv8872_get(uint8_t index) {
    if (index >= 3) return NULL;
    return &drv_array[index];
}

// === Contrôles basiques ===
void drv8872_forward(drv8872_t* drv) {
    gpio_put(drv->in1_pin, 1);
    gpio_put(drv->in2_pin, 0);
}

void drv8872_reverse(drv8872_t* drv) {
    gpio_put(drv->in1_pin, 0);
    gpio_put(drv->in2_pin, 1);
}

void drv8872_brake(drv8872_t* drv) {
    gpio_put(drv->in1_pin, 1);
    gpio_put(drv->in2_pin, 1);
}

void drv8872_stop(drv8872_t* drv) {
    gpio_put(drv->in1_pin, 0);
    gpio_put(drv->in2_pin, 0);
}

// === Contrôles PWM ===
void drv8872_forward_pwm(drv8872_t* drv, uint slice_num, uint level) {
    gpio_set_function(drv->in1_pin, GPIO_FUNC_PWM);
    gpio_put(drv->in2_pin, 0);
    pwm_set_gpio_level(drv->in1_pin, level);
    pwm_set_enabled(slice_num, true);
}

void drv8872_reverse_pwm(drv8872_t* drv, uint slice_num, uint level) {
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
    if (!drv->fault_callback) return;
    gpio_set_irq_enabled_with_callback(drv->fault_pin, GPIO_IRQ_EDGE_FALL, true, gpio_fault_callback);
}

// === Activation groupe avec timer ===
static int64_t drv8872_group_alarm_callback(alarm_id_t id, void* user_data) {
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
    drv8872_init(&motor1);
    drv8872_init(&motor2);
    drv8872_init(&motor3);
}

void motor_diag_log_faults(void) {
    debug_printf("[MOTOR_DIAG] Fault M1: %s\n", drv8872_is_fault(&motor1) ? "YES" : "NO");
    debug_printf("[MOTOR_DIAG] Fault M2: %s\n", drv8872_is_fault(&motor2) ? "YES" : "NO");
    debug_printf("[MOTOR_DIAG] Fault M3: %s\n", drv8872_is_fault(&motor3) ? "YES" : "NO");
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
