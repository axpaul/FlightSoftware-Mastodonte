// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : drv8872.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Driver DRV8872 (contrôle moteur H-bridge)
// -------------------------------------------------------------
 
#include "drv8872.h"             

// === Définition de 3 instances de moteurs globalement accessibles ===

// Motor 1
drv8872_t motor1 = {
    .in1_pin = IN1_M1,
    .in2_pin = IN2_M1,
    .fault_pin = NFAUT_M1,
    .fault_callback = NULL
};

// Motor 2
drv8872_t motor2 = {
    .in1_pin = IN1_M2,
    .in2_pin = IN2_M2,
    .fault_pin = NFAUT_M2,
    .fault_callback = NULL
};

// Motor 3
drv8872_t motor3 = {
    .in1_pin = IN1_M3,
    .in2_pin = IN2_M3,
    .fault_pin = NFAUT_M3,
    .fault_callback = NULL
};

// Tableau global pour accéder aux moteurs facilement par index (0, 1, 2)
drv8872_t* motors[3] = { &motor1, &motor2, &motor3 };

// Définition du groupe 
drv8872_group_t group_all_motors = {
    .motors = { &motor1, &motor2, &motor3 },
    .motor_count = 3,
    .direction = true // Marche avant par défaut, modifiable ensuite
};

// === Instance statique alternative (utilisée par drv8872_get) ===
// Peut servir pour des accès isolés sans global extern
static drv8872_t drv_array[3] = {
    { IN1_M1, IN2_M1, NFAUT_M1, NULL },
    { IN1_M2, IN2_M2, NFAUT_M2, NULL },
    { IN1_M3, IN2_M3, NFAUT_M3, NULL },
};

// === Structure utilisée pour passer un groupe de moteurs à l'alarme ===
typedef struct {
    drv8872_group_t group;
} drv_group_timer_context_t;

// === Pointeur global pour le callback de nFAULT (un seul supporté ici) ===
static drv8872_t* _drv_ref = NULL;

// === Callback d'interruption sur nFAULT ===
// Appelé lors d’un front descendant (FAULT LOW)
static void gpio_fault_callback(uint gpio, uint32_t events) {
    if (_drv_ref && _drv_ref->fault_callback) {
        _drv_ref->fault_callback();
    }
}

// === Initialisation simple (pas de configuration GPIO ici) ===
void drv8872_init(drv8872_t* drv) {
    drv8872_stop(drv);   // On commence avec moteur à l’arrêt
    _drv_ref = drv;      // Enregistre le pointeur pour le handler FAULT
}

// === Accès à l’un des 3 moteurs via index ===
drv8872_t* drv8872_get(uint8_t index) {
    if (index >= 3) return NULL;
    return &drv_array[index];
}

// === Commandes simples ===
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

// === Commandes PWM (utilise 1 seul canal de PWM sur 1 broche) ===
void drv8872_forward_pwm(drv8872_t* drv, uint slice_num, uint level) {
    gpio_set_function(drv->in1_pin, GPIO_FUNC_PWM);  // Active le PWM sur IN1
    gpio_put(drv->in2_pin, 0);                       // IN2 reste LOW
    pwm_set_gpio_level(drv->in1_pin, level);         // Définit le niveau PWM
    pwm_set_enabled(slice_num, true);                // Active le PWM
}

void drv8872_reverse_pwm(drv8872_t* drv, uint slice_num, uint level) {
    gpio_set_function(drv->in2_pin, GPIO_FUNC_PWM);  // Active le PWM sur IN2
    gpio_put(drv->in1_pin, 0);                       // IN1 reste LOW
    pwm_set_gpio_level(drv->in2_pin, level);         // Définit le niveau PWM
    pwm_set_enabled(slice_num, true);                // Active le PWM
}

// === Diagnostic : vérifie l'état de la ligne nFAULT ===
bool drv8872_is_fault(drv8872_t* drv) {
    return gpio_get(drv->fault_pin) == 0;  // Actif à LOW
}

// === Active une interruption sur nFAULT (chute de tension) ===
void drv8872_setup_fault_interrupt(drv8872_t* drv) {
    if (!drv->fault_callback) return;
    gpio_set_irq_enabled_with_callback(drv->fault_pin, GPIO_IRQ_EDGE_FALL, true, gpio_fault_callback);
}

// === Callback appelé par le timer après délai d’activation ===
static int64_t drv8872_group_alarm_callback(alarm_id_t id, void* user_data) {
    drv_group_timer_context_t* ctx = (drv_group_timer_context_t*)user_data;
    if (ctx) {
        for (uint8_t i = 0; i < ctx->group.motor_count; i++) {
            if (ctx->group.motors[i]) {
                drv8872_stop(ctx->group.motors[i]);  // Arrête chaque moteur
            }
        }
        free(ctx);  // Libère la mémoire du contexte
    }
    return 0;
}

// === Active un ou plusieurs moteurs pendant un délai défini ===
void drv8872_group_activate_for_us(drv8872_group_t* group, uint64_t duration_us) {
    if (!group || group->motor_count == 0) return;

    // Active les moteurs dans la bonne direction
    for (uint8_t i = 0; i < group->motor_count; i++) {
        if (group->motors[i]) {
            if (group->direction) {
                drv8872_forward(group->motors[i]);
            } else {
                drv8872_reverse(group->motors[i]);
            }
        }
    }

    // Crée un contexte pour arrêter le groupe après le délai
    drv_group_timer_context_t* ctx = (drv_group_timer_context_t*) malloc(sizeof(drv_group_timer_context_t));
    if (!ctx) 
        return;

    ctx->group = *group;  // Copie le contenu de la structure
    add_alarm_in_us(duration_us, drv8872_group_alarm_callback, ctx, true);  // Démarre le timer
}

void motor_diag_init(void) {
    drv8872_init(&motor1);
    drv8872_init(&motor2);
    drv8872_init(&motor3);
}

void motor_diag_log_faults(void) {
    bool f1 = drv8872_is_fault(&motor1);
    bool f2 = drv8872_is_fault(&motor2);
    bool f3 = drv8872_is_fault(&motor3);

    debug_printf("[MOTOR_DIAG] Fault M1: %s\n", f1 ? "YES" : "NO");
    debug_printf("[MOTOR_DIAG] Fault M2: %s\n", f2 ? "YES" : "NO");
    debug_printf("[MOTOR_DIAG] Fault M3: %s\n", f3 ? "YES" : "NO");
}

void motor_diag_test_sequence(void) {
    debug_println("[MOTOR_DIAG] Start test sequence");

    // Marche avant
    drv8872_forward(&motor1);
    drv8872_forward(&motor2);
    drv8872_forward(&motor3);
    debug_println("[MOTOR_DIAG] Forward");
    sleep_ms(5000);
    motor_diag_log_faults();

    // Marche arrière
    drv8872_reverse(&motor1);
    drv8872_reverse(&motor2);
    drv8872_reverse(&motor3);
    debug_println("[MOTOR_DIAG] Reverse");
    sleep_ms(5000);
    motor_diag_log_faults();

    // Frein
    drv8872_brake(&motor1);
    drv8872_brake(&motor2);
    drv8872_brake(&motor3);
    debug_println("[MOTOR_DIAG] Brake");
    sleep_ms(1000);
    motor_diag_log_faults();

    // Stop
    drv8872_stop(&motor1);
    drv8872_stop(&motor2);
    drv8872_stop(&motor3);
    debug_println("[MOTOR_DIAG] Stop");
    sleep_ms(1000);
    motor_diag_log_faults();

    debug_println("[MOTOR_DIAG] Test complete");
}