// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "sequencer.h"

rocket_state_t currentState = PRE_FLIGHT;
//absolute_time_t windowStartTime;
alarm_id_t windowAlarmId = -1;

volatile uint8_t triggerRBF = 0;
volatile uint8_t triggerJack = 0;
volatile uint8_t triggerOcto3 = 0;
volatile uint8_t triggerOcto4 = 0;
volatile uint8_t triggerTouch = 0;

bool windowOpen = false;

void seq_gpio_callback(uint gpio, uint32_t events) {
    timestamp_t ts = compute_timestamp(get_absolute_time());
    debug_printf("[INTERRUPT] GPIO %d Event @ %02lu:%02lu.%03lu.%03lu\n",
                 gpio, ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[INTERRUPT] Event mask = 0x%08X\n", events);

    switch (gpio) {
        case PIN_SMITCH_N2:
            if ((events & GPIO_IRQ_EDGE_FALL) &&
                (currentState == PRE_FLIGHT || currentState == PYRO_RDY)) {
                triggerRBF = 1;
                gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_FALL);
                debug_println("[INTERRUPT] RBF disarmed → transition to PYRO_RDY");
            }
    
            if (events & GPIO_IRQ_EDGE_RISE) {
                triggerRBF = 2;
                gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_RISE);
                debug_println("[INTERRUPT] RBF inserted → return to PRE_FLIGHT");
            }
            break;
    
        case PIN_SMITCH_N1:
            if ((events & GPIO_IRQ_EDGE_FALL) && currentState == PYRO_RDY) {
                triggerJack = 1;
                gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_FALL);
                debug_println("[INTERRUPT] Jack removed → LIFTOFF detected");
            }
            break;
    
        case PIN_OCTO_N3:
            if ((events & GPIO_IRQ_EDGE_RISE) && currentState == WINDOW) {
                triggerOcto3 = 1;
                gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_RISE);
                debug_println("[INTERRUPT] Octo N3 triggered (HIGH)");
            }
            break;
    
        case PIN_OCTO_N4:
            if ((events & GPIO_IRQ_EDGE_RISE) && currentState == WINDOW) {
                triggerOcto4 = 1;
                gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_RISE);
                debug_println("[INTERRUPT] Octo N4 triggered (HIGH)");
            }
            break;
    }
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

int64_t seq_touchdown_timeout_callback(alarm_id_t id, void* user_data){
    timestamp_t ts = compute_timestamp(get_absolute_time());

    debug_printf("[INTERRUPT] Close touchdown timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);

    triggerTouch = 1;

    return 0;
}

void seq_reset_triggers() {
    triggerRBF = 0;
    triggerJack = 0;
    triggerOcto3 = 0;
    triggerOcto4 = 0;
    triggerTouch = 0;
}
  
rocket_state_t seq_init(void){

    uint8_t jackState = gpio_get(PIN_SMITCH_N1);
    uint8_t rbfState  = gpio_get(PIN_SMITCH_N2);
    uint8_t octo3State = gpio_get(PIN_OCTO_N3);
    uint8_t octo4State = gpio_get(PIN_OCTO_N4);

    debug_printf("[CHECK] Pin 26 (JACK)  = %d (%s)\n", jackState,  (jackState == 0 ? "REMOVED" : "CONTINUITY"));
    debug_printf("[CHECK] Pin 27 (RBF)   = %d (%s)\n", rbfState,   (rbfState  == 0 ? "REMOVED"     : "INSERTED"));
    debug_printf("[CHECK] Pin 3 (OCTO 1)   = %d (%s)\n", octo3State,   (octo3State  == 0 ? "LOW"     : "HIGH"));
    debug_printf("[CHECK] Pin 4 (OCTO 2)   = %d (%s)\n", octo4State,   (octo4State  == 0 ? "LOW"     : "HIGH"));

    gpio_set_irq_callback(seq_gpio_callback);

    gpio_set_irq_enabled(PIN_SMITCH_N2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE, true);

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
            break;

        // === État : Descente sous parachute ===
        case DESCEND:
            currentState = seq_descend();
            break;

        // === État : Atterrissage ===
        case TOUCHDOWN:
            currentState = seq_touchdown();
            break;

        // === État : Erreur (sécurité) ===
        default:
            currentState = ERROR_SEQ;
            break;
    }

  return currentState;
}

rocket_state_t seq_preLaunch(void) {
    // Passage à PYRO_RDY après retrait du RBF
    if (triggerRBF == 1) {  
        triggerRBF = 0;       

        apply_state_config(PYRO_RDY);
        return PYRO_RDY;
    }
    
    return PRE_FLIGHT;
}

rocket_state_t seq_pyroRdy(void){
    // Passage à PRE_FLIGHT après insertion du RBF
    if (triggerRBF == 2) {
        triggerRBF = 0;

        apply_state_config(PRE_FLIGHT);
        return PRE_FLIGHT;
    }
    // Passage à ASCEND après retrait du Jack
    if (triggerJack == 1) {
        triggerJack = 0;
        timestamp_t ts = compute_timestamp(get_absolute_time());

        gpio_acknowledge_irq(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL);
        gpio_acknowledge_irq(PIN_SMITCH_N2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
        gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, false);
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

rocket_state_t seq_window(void) {
    
    // --- Cas 1 : Fin de fenêtre par timer ---
    if (windowOpen == false) {
        timestamp_t ts = compute_timestamp(get_absolute_time());
        if (windowAlarmId != -1) {
            cancel_alarm(windowAlarmId);
            debug_println("[WINDOW] Stop Window alarm");
        }

        debug_printf("[WINDOW] Alarm close timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        debug_println("[WINDOW] Start deployment on timer");

        gpio_acknowledge_irq(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE);
        gpio_acknowledge_irq(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE);
        gpio_set_irq_enabled(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE, false);

        apply_state_config(DEPLOY_TIMER);
        return DEPLOY_TIMER;
    }

    // --- Cas 2 : Détection par algo (Octo3 ou Octo4) ---
    if (triggerOcto3 == 1 || triggerOcto4 == 1) {
        timestamp_t ts = compute_timestamp(get_absolute_time());
        triggerOcto3 = 0;
        triggerOcto4 = 0;

        if (windowAlarmId != -1) {
            cancel_alarm(windowAlarmId);
            debug_println("[WINDOW] Stop Window alarm");
        }

        debug_printf("[WINDOW] Start deployment on algo @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);

        gpio_acknowledge_irq(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE);
        gpio_acknowledge_irq(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE);
        gpio_set_irq_enabled(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE, false);

        apply_state_config(DEPLOY_ALGO);
        return DEPLOY_ALGO;
    }

    return WINDOW;
}


rocket_state_t seq_deploy(void){

    timestamp_t ts = compute_timestamp(get_absolute_time());
    add_alarm_in_us(THEORETICAL_DESCENT_US, seq_touchdown_timeout_callback, nullptr, true);
    debug_printf("[DEPLOY] Alarm start timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[DEPLOY] Touchdown expeted in %.2f s\n", THEORETICAL_DESCENT_US / 1e6);

    apply_state_config(DESCEND);
    return DESCEND;
}

rocket_state_t seq_descend(void){

    if (triggerTouch == 1){
        timestamp_t ts = compute_timestamp(get_absolute_time());

        debug_printf("[DESCEND] Touchdown @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        
        apply_state_config(TOUCHDOWN);
        return TOUCHDOWN;

    }

    return DESCEND;
}

rocket_state_t seq_touchdown(void){

    return TOUCHDOWN;
}


