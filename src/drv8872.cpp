#include "drv8872.h"
#include "hardware/pwm.h"

static drv8872_t* _drv_ref = NULL; // Référence globale pour callback simple

// IRQ Handler interne
static void gpio_fault_callback(uint gpio, uint32_t events) {
    if (_drv_ref && _drv_ref->fault_callback) {
        _drv_ref->fault_callback();
    }
}

void drv8872_init(drv8872_t* drv) {
    gpio_init(drv->in1_pin);
    gpio_init(drv->in2_pin);
    gpio_set_dir(drv->in1_pin, GPIO_OUT);
    gpio_set_dir(drv->in2_pin, GPIO_OUT);
    drv8872_stop(drv);

    // Enregistrer la structure pour l'interruption
    _drv_ref = drv;

    if (drv->fault_pin != 0xFFFFFFFF) {
        gpio_init(drv->fault_pin);
        gpio_set_dir(drv->fault_pin, GPIO_IN);
        gpio_pull_up(drv->fault_pin); // nFAULT est open-drain, donc pull-up
    }
}

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

bool drv8872_is_fault(drv8872_t* drv) {
    if (drv->fault_pin == 0xFFFFFFFF) return false;
    return gpio_get(drv->fault_pin) == 0; // Active LOW
}

void drv8872_setup_fault_interrupt(drv8872_t* drv) {
    if (drv->fault_pin == 0xFFFFFFFF || !drv->fault_callback) return;
    gpio_set_irq_enabled_with_callback(drv->fault_pin, GPIO_IRQ_EDGE_FALL, true, &gpio_fault_callback);
}
