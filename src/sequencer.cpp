// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "sequencer.h"

rocket_state_t currentState = PRE_FLIGHT;
absolute_time_t windowStartTime;
alarm_id_t windowAlarmId = -1;

volatile uint8_t triggerRBF = 0;
volatile uint8_t triggerJack = 0;
bool windowOpen = false;

void seq_gpio_callback(uint gpio, uint32_t events) {
    timestamp_t ts = compute_timestamp(get_absolute_time());
    uint8_t value = gpio_get(gpio);

    debug_printf("[INTERRUPT] GPIO %d Event @ %02lu:%02lu.%03lu.%03lu\n",
                 gpio, ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[INTERRUPT] Pin value = %d (logic %s)\n", value, (value == 0 ? "LOW" : "HIGH"));

    if (gpio == PIN_SMITCH_N2) {
        if (value == 0) {
            triggerRBF = 1;
            debug_println("[INTERRUPT] RBF disarmed → transition to PYRO_RDY");
        } else {
            triggerRBF = 2;
            debug_println("[INTERRUPT] RBF inserted → return to PRE_FLIGHT");
        }
    } else if (gpio == PIN_SMITCH_N1) {
        if (value == 0) {
            triggerJack = 1;
            debug_println("[INTERRUPT] Jack removed → LIFTOFF detected");
        } else {
            triggerJack = 0;
            debug_println("[INTERRUPT] Jack present → CONTINUITY detected");
        }
    }

    gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
}
  
int64_t seq_is_window_open_callback(alarm_id_t id, void* user_data) {
    timestamp_t ts = compute_timestamp(get_absolute_time());

    debug_printf("[INTERRUPT] Open window timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[INTERRUPT] Expected at T+%.2f s\n", WINDOW_OPEN_OFFSET_US / 1e6);

    windowOpen = true;

    debug_println("[SEQ] -10 percent apogee reached → WINDOW state activated");
    return 0;
}

int64_t seq_window_timeout_callback(alarm_id_t id, void* user_data) {
    timestamp_t ts = compute_timestamp(get_absolute_time());

    debug_printf("[INTERRUPT] Close window timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[INTERRUPT] Expected at T+%.2f s\n", WINDOW_OPEN_OFFSET_US / 1e6);

    windowOpen = false;

    debug_println("[SEQ] +10 percent climax reached → WINDOW state disabled");
    return 0;
  }

void seq_reset_triggers() {
    triggerRBF = 0;
    triggerJack = 0;
}
  
rocket_state_t seq_init(void){

    uint8_t jackState = gpio_get(PIN_SMITCH_N1);
    uint8_t rbfState  = gpio_get(PIN_SMITCH_N2);

    debug_printf("[CHECK] Pin N1 (JACK)  = %d (%s)\n", jackState,  (jackState == 0 ? "REMOVED" : "CONTINUITY"));
    debug_printf("[CHECK] Pin N2 (RBF)   = %d (%s)\n", rbfState,   (rbfState  == 0 ? "REMOVED"     : "INSERTED"));

    gpio_set_irq_callback(seq_gpio_callback);

    gpio_set_irq_enabled(PIN_SMITCH_N2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    irq_set_enabled(IO_IRQ_BANK0, true);

    apply_state_config(PRE_FLIGHT);
    return PRE_FLIGHT;
}

rocket_state_t seq_handle(void) {
    switch (currentState) {

    // === État : Pré-vol ===
        case PRE_FLIGHT:
            currentState = seq_preLaunch();
            break;

        // === État : Prêt pyrotechnique ===
        case PYRO_RDY:
            currentState = seq_pyroRdy();
            break;

        // === État : Ascension ===
        case ASCEND:
            currentState = seq_ascend();
            break;

        // === État : Fenetre ===
        case WINDOW: 
            currentState = seq_window();
            break;

        // === État : Déploiement via algo ===
        case DEPLOY_ALGO:
            currentState = seq_deploy();
            break;

        // === État : Déploiement via timer ===
        case DEPLOY_TIMER:
            currentState = seq_deploy(); // logique partagée
            apply_state_config(DEPLOY_TIMER);
            break;

        // === État : Descente sous parachute ===
        case DESCEND:
            currentState = seq_descend();
            apply_state_config(DESCEND);
            break;

        // === État : Atterrissage ===
        case TOUCHDOWN:
            currentState = seq_touchdown();
            apply_state_config(TOUCHDOWN);
            break;

        // === État : Erreur (sécurité) ===
        default:
            currentState = ERROR_SEQ;
            apply_state_config(ERROR_SEQ);
            break;
    }

  return currentState;
}

rocket_state_t seq_preLaunch(void) {
    // Passage à PYRO_RDY après retrait du RBF
    if (triggerRBF == 1) {  
        triggerRBF = 0;

        gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, true);

        apply_state_config(PYRO_RDY);
        return PYRO_RDY;
    }
    
    return PRE_FLIGHT;
}

rocket_state_t seq_pyroRdy(void){
    // Passage à PRE_FLIGHT après insertion du RBF
    if (triggerRBF == 2) {
        triggerRBF = 0;

        gpio_acknowledge_irq(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL); 
        gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, false);

        apply_state_config(PRE_FLIGHT);
        return PRE_FLIGHT;
    }
    // Passage à ASCEND après retrait du Jack
    if (triggerJack == 1) {
        triggerJack = 0;
        timestamp_t ts = compute_timestamp(get_absolute_time());

        gpio_acknowledge_irq(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL); 
        gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, false);

        gpio_acknowledge_irq(PIN_SMITCH_N2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
        gpio_set_irq_enabled(PIN_SMITCH_N2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

        add_alarm_in_us(WINDOW_OPEN_OFFSET_US, seq_is_window_open_callback, nullptr, true);

        debug_printf("[PYRO_RDY] Alarm start timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        debug_printf("[PYRO_RDY] Window scheduled to open at T+%.2f s, duration %.2f s\n", WINDOW_OPEN_OFFSET_US / 1e6, WINDOW_DURATION_US / 1e6);
        
        apply_state_config(ASCEND);
        return ASCEND;
    }
    return PYRO_RDY;
}


rocket_state_t seq_ascend(void){

    if(windowOpen == true){
        timestamp_t ts = compute_timestamp(get_absolute_time());
        if (windowAlarmId != -1) {
            cancel_alarm(windowAlarmId);
        }

        windowAlarmId = add_alarm_in_us(WINDOW_DURATION_US, seq_window_timeout_callback, NULL, true);
        debug_printf("[ASCEND] Alarm start timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        debug_printf("[ASCEND] Window closure timer started for %.2f s\n", WINDOW_DURATION_US / 1e6);

        apply_state_config(WINDOW);

        return WINDOW;
    }

    return ASCEND;
}

rocket_state_t seq_window(void){

    if(windowOpen == false){
        timestamp_t ts = compute_timestamp(get_absolute_time());
        if (windowAlarmId != -1) {
            cancel_alarm(windowAlarmId);
        }

        windowAlarmId = add_alarm_in_us(WINDOW_DURATION_US, seq_window_timeout_callback, NULL, true);
        debug_printf("[WINDOW] Alarm close timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);

        apply_state_config(DESCEND);

        return DESCEND;
    }

    return WINDOW;
}

rocket_state_t seq_deploy(void){

    return PRE_FLIGHT;
}

rocket_state_t seq_descend(void){

    return DESCEND;
}

rocket_state_t seq_touchdown(void){

    return PRE_FLIGHT;
}

void apply_state_config(rocket_state_t state) {
    uint32_t color = 0;
    uint16_t freq = 2000;   // Fréquence standard du buzzer
    uint16_t freqL = 300;   // Fréquence "low", plus grave (pour feedback discret)

    switch (state) {

        // === État : Pré-vol ===
        case PRE_FLIGHT:
            color = rgb.Color(0, 255, 0);             // Vert : prêt au sol
            setBuzzer(true, 3000, 200, freqL);        // Double bip très espacé, discret
            break;

        // === État : Pyro prêt ===
        case PYRO_RDY:
            color = rgb.Color(255, 255, 0);           // Jaune : Moteur prêt
            setBuzzer(true, 1000, 500, freqL);        // Bip lent, 1 bip/sec
            break;

        // === État : Ascension ===
        case ASCEND:
            color = rgb.Color(0, 0, 255);             // Bleu : en montée
            setBuzzer(true, 100, 75, freqL);          // Bip ultra rapide
            break;

        // === État : Fenêtre d’ouverture (timer ou algo) ===
        case WINDOW:
            color = rgb.Color(0, 255, 255);           // Cyan : fenêtre active
            setBuzzer(true, 400, 300, freqL);         // Bip rapide (alerte)
            break;

        // === État : Déploiement par algo ou timer ===
        case DEPLOY_ALGO:
        case DEPLOY_TIMER:
            color = rgb.Color(255, 165, 0);           // Orange : déploiement
            setBuzzer(true, 400, 400, freqL);         // Bip continu mais intermittent
            break;

        // === État : Descente sous parachute ===
        case DESCEND:
            color = rgb.Color(255, 0, 255);           // Magenta : descente
            setBuzzer(true, 1000, 200, freqL);        // Bip lent et régulier
            break;

        // === État : Atterrissage ===
        case TOUCHDOWN:
            color = rgb.Color(0, 255, 0);             // Vert fixe : au sol, OK
            setBuzzer(true, 60000, 5000, freqL);      // Bip long toutes les 60 secondes
            break;

        // === État : Erreur système / séquence ===
        case ERROR_SEQ:
            color = rgb.Color(255, 0, 0);             // Rouge : erreur
            setBuzzer(true, 500, 250, 500);           // Bip rapide et aigu
            break;
    }

    rgb.setPixelColor(0, color);
    rgb.show();
}


// void seq_arm_rbf_callback(void) {
//     timestamp_t ts = compute_timestamp(get_absolute_time());
//     uint8_t GPIO = gpio_get(PIN_SMITCH_N2);

//     debug_printf("[INTERRUPT] RBF Event @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
//     debug_printf("[INTERRUPT] Pin value = %d (logic %s)\n", GPIO, (GPIO == 0 ? "LOW" : "HIGH"));

//     if (GPIO == 0) {
//         triggerRBF = 1;
//         debug_println("[SEQ] RBF disarmed → transition to PYRO_RDY");
//     }

//     if (GPIO == 1) {
//         triggerRBF = 2;
//         debug_println("[SEQ] RBF inserted → return to PRE_FLIGHT");
//     }

//     gpio_acknowledge_irq(PIN_SMITCH_N2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
// }

// void seq_detect_liftoff_callback(void) {
//     timestamp_t ts = compute_timestamp(get_absolute_time());
//     uint8_t GPIO = gpio_get(PIN_SMITCH_N1);

//     debug_printf("[INTERRUPT] Jack Detected @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
//     debug_printf("[INTERRUPT] Pin value = %d (logic %s)\n", GPIO, (GPIO == 0 ? "LOW" : "HIGH"));

//     if (GPIO == 0) {
//         triggerJack = 1;
//         debug_println("[SEQ] Jack removed → LIFTOFF detected");
//     }

//     gpio_acknowledge_irq(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL);
// }
