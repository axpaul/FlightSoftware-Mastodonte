// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "sequencer.h"
#include "lps22hb.h"

rocket_state_t currentState = PRE_FLIGHT;
alarm_id_t windowAlarmId = -1;

volatile uint8_t triggerRBF = 0;
volatile uint8_t triggerJack = 0;
volatile uint8_t triggerOcto3 = 0;
volatile uint8_t triggerOcto4 = 0;
volatile uint8_t triggerTouch = 0;
volatile uint8_t triggerBaroApogee = 0;
volatile bool windowHasOpened = false;

float ground_pressure = -1.0f;

// Variables d'état du Filtre de Kalman 1D
static float kalman_z = 0.0f;  // Altitude estimée (m)
static float kalman_v = 0.0f;  // Vitesse verticale estimée (m/s)
static float P_cov[2][2] = {{1.0f, 0.0f}, {0.0f, 1.0f}}; // Covariance d'erreur

// Paramètres de bruit du filtre
static const float Q_alt = 0.1f;   // Incertitude sur l'altitude
static const float Q_vel = 0.5f;   // Incertitude sur la vitesse
static const float R_alt = 1.0f;   // Variance du bruit de mesure du baromètre
static uint32_t apogee_counter = 0;

bool windowOpen = false;

void seq_gpio_callback(uint gpio, uint32_t events) {
    switch (gpio) {
        case PIN_SMITCH_N2:
            if ((events & GPIO_IRQ_EDGE_FALL) &&
                (currentState == PRE_FLIGHT || currentState == PYRO_RDY)) {
                triggerRBF = 1;
            }
            if (events & GPIO_IRQ_EDGE_RISE) {
                triggerRBF = 2;
            }
            break;
    
        case PIN_SMITCH_N1:
            if ((events & GPIO_IRQ_EDGE_FALL) && (currentState == PYRO_RDY)) {
                triggerJack = 1;
            }
            if (events & GPIO_IRQ_EDGE_RISE) {
                triggerJack = 0;
            }
            break;
    
        case PIN_OCTO_N3:
            if ((events & GPIO_IRQ_EDGE_RISE) && (currentState == WINDOW)) {
                triggerOcto3 = 1;
            }
            break;
    
        case PIN_OCTO_N4:
            if ((events & GPIO_IRQ_EDGE_RISE) && (currentState == WINDOW)) {
                triggerOcto4 = 1;
            }
            break;

        default:
            break;
    }
}    

int64_t seq_is_window_open_callback(alarm_id_t id, void* user_data) {
    windowOpen = true;
    windowHasOpened = true;
    return 0;
}

int64_t seq_window_timeout_callback(alarm_id_t id, void* user_data) {
    windowOpen = false;
    return 0;
}

int64_t seq_touchdown_timeout_callback(alarm_id_t id, void* user_data){
    triggerTouch = 1;
    return 0;
}

void seq_reset_triggers() {
    triggerRBF = 0;
    triggerJack = 0;
    triggerOcto3 = 0;
    triggerOcto4 = 0;
    triggerTouch = 0;
    triggerBaroApogee = 0;
    windowHasOpened = false;
}
  
rocket_state_t seq_init(void){

    uint8_t jackState = gpio_get(PIN_SMITCH_N1);
    uint8_t rbfState  = gpio_get(PIN_SMITCH_N2);
    uint8_t octo3State = gpio_get(PIN_OCTO_N3);
    uint8_t octo4State = gpio_get(PIN_OCTO_N4);

    log_entryf("[CHECK] Initial GPIO states: JACK=%d, RBF=%d, OCTO3=%d, OCTO4=%d", jackState, rbfState, octo3State, octo4State);

    // Initialisation des variables Kalman
    kalman_z = 0.0f;
    kalman_v = 0.0f;
    P_cov[0][0] = 1.0f; P_cov[0][1] = 0.0f;
    P_cov[1][0] = 0.0f; P_cov[1][1] = 1.0f;
    apogee_counter = 0;
    triggerBaroApogee = 0;

    gpio_set_irq_enabled(PIN_SMITCH_N2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_RISE, true);
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

        // === État : Erreur moteur ===
        case ERROR_MOTOR:
            break;

        // === État : Erreur (sécurité) ===
        case ERROR_SEQ:
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
        debug_println("[SEQ] RBF disarmed (removed) → transition to PYRO_RDY");
        log_entry("[SEQ] RBF removed");

        apply_state_config(PYRO_RDY);
        return PYRO_RDY;
    }
    
    return PRE_FLIGHT;
}

rocket_state_t seq_pyroRdy(void){
    // Passage à PRE_FLIGHT après insertion du RBF
    if (triggerRBF == 2) {
        triggerRBF = 0;
        debug_println("[SEQ] RBF inserted → return to PRE_FLIGHT");
        log_entry("[SEQ] RBF inserted");

        apply_state_config(PRE_FLIGHT);
        return PRE_FLIGHT;
    }
    // Passage à ASCEND après retrait du Jack
    if (triggerJack == 1) {
        triggerJack = 0;
        timestamp_t ts = compute_timestamp(get_absolute_time());

        gpio_acknowledge_irq(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL);
        gpio_acknowledge_irq(PIN_SMITCH_N1, GPIO_IRQ_EDGE_RISE);
        gpio_acknowledge_irq(PIN_SMITCH_N2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

        gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_FALL, false);
        gpio_set_irq_enabled(PIN_SMITCH_N1, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(PIN_SMITCH_N2, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

        // Désactivation et acquittement des interruptions de défaut moteurs pour le vol 
        // (évite que le pic d'appel de courant au démarrage ne coupe les moteurs)
        gpio_acknowledge_irq(NFAUT_M1, GPIO_IRQ_EDGE_FALL);
        gpio_acknowledge_irq(NFAUT_M2, GPIO_IRQ_EDGE_FALL);
        gpio_acknowledge_irq(NFAUT_M3, GPIO_IRQ_EDGE_FALL);
        gpio_set_irq_enabled(NFAUT_M1, GPIO_IRQ_EDGE_FALL, false);
        gpio_set_irq_enabled(NFAUT_M2, GPIO_IRQ_EDGE_FALL, false);
        gpio_set_irq_enabled(NFAUT_M3, GPIO_IRQ_EDGE_FALL, false);

        add_alarm_in_us(WINDOW_OPEN_OFFSET_US, seq_is_window_open_callback, nullptr, true);

        debug_println("[PYRO_RDY] Jack removed → LIFTOFF detected");
        log_entry("[PYRO_RDY] Jack removed");

        debug_printf("[PYRO_RDY] Alarm start timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        debug_printf("[PYRO_RDY] Window scheduled to open at T+%.2f s, duration %.2f s\n", WINDOW_OPEN_OFFSET_US / 1e6, WINDOW_DURATION_US / 1e6);

        apply_state_config(ASCEND);
        return ASCEND;
    }
    return PYRO_RDY;
}


rocket_state_t seq_ascend(void){

    if(windowHasOpened == true){
        timestamp_t ts = compute_timestamp(get_absolute_time());
        if (windowAlarmId != -1) {
            cancel_alarm(windowAlarmId);
        }

        windowAlarmId = add_alarm_in_us(WINDOW_DURATION_US, seq_window_timeout_callback, NULL, true);
        
        debug_printf("[ASCEND] Alarm start timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        debug_printf("[ASCEND] Window closure timer started for %.2f s\n", WINDOW_DURATION_US / 1e6);
        log_entry("[ASCEND] Window opened");

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
        log_entry("[WINDOW] Window closed");

        apply_state_config(DEPLOY_TIMER);
        return DEPLOY_TIMER;
    }

    // --- Cas 2 : Détection par algo (Octo3, Octo4 ou Baromètre) ---
    if (triggerOcto3 == 1 || triggerOcto4 == 1 || triggerBaroApogee == 1) {
        timestamp_t ts = compute_timestamp(get_absolute_time());
        uint8_t triggered_sensor = 0;
        if (triggerOcto3 == 1) triggered_sensor = 3;
        else if (triggerOcto4 == 1) triggered_sensor = 4;
        else triggered_sensor = 5; // 5 = Baromètre (Kalman)
        
        triggerOcto3 = 0;
        triggerOcto4 = 0;
        triggerBaroApogee = 0;

        if (windowAlarmId != -1) {
            cancel_alarm(windowAlarmId);
            debug_println("[WINDOW] Stop Window alarm");
        }

        if (triggered_sensor == 5) {
            debug_printf("[WINDOW] Barometer apogee triggered @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
            log_entry("[WINDOW] Apogee detected by Kalman filter");
        } else {
            debug_printf("[WINDOW] Octo N%d triggered (HIGH) @ %02lu:%02lu.%03lu.%03lu\n", triggered_sensor, ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
            log_entryf("[WINDOW] Climax detected on Octo N%d", triggered_sensor);
        }
        
        debug_printf("[WINDOW] Start deployment on algo @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
        apply_state_config(DEPLOY_ALGO);
        return DEPLOY_ALGO;
    }

    return WINDOW;
}

rocket_state_t seq_deploy(void) {
    timestamp_t ts = compute_timestamp(get_absolute_time());

    // === Déclenchement des moteurs de déploiement ===
    group_all_motors.direction = true;  // Direction normale = déploiement
    debug_println("[DEPLOY] Deployment motors activated for 3 seconds");
    drv8872_group_activate_for_us(&group_all_motors, 3000000); // 3s

    // === Démarrage timer d'atterrissage estimé ===
    add_alarm_in_us(THEORETICAL_DESCENT_US, seq_touchdown_timeout_callback, nullptr, true);
    debug_printf("[DEPLOY] Alarm start timer @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[DEPLOY] Touchdown expected in %.2f s\n", THEORETICAL_DESCENT_US / 1e6);
    
    log_entry("[DEPLOY] Motors activated & Touchdown timer started");
    apply_state_config(DESCEND);
    return DESCEND;
}


rocket_state_t seq_descend(void){

    if (triggerTouch == 1){
        timestamp_t ts = compute_timestamp(get_absolute_time());

        debug_printf("[DESCEND] Touchdown detected @ %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);

        gpio_acknowledge_irq(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE);
        gpio_acknowledge_irq(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE);
        gpio_set_irq_enabled(PIN_OCTO_N3, GPIO_IRQ_EDGE_RISE, false);
        gpio_set_irq_enabled(PIN_OCTO_N4, GPIO_IRQ_EDGE_RISE, false);
    
        log_entry("[SEQ] Touchdown detected");
        apply_state_config(TOUCHDOWN);
        return TOUCHDOWN;
    }

    return DESCEND;
}

rocket_state_t seq_touchdown(void){
    return TOUCHDOWN;
}

void seq_baro_drdy_callback(void) {
    if (!baro_present) {
        return; // Bypass de sécurité si le baromètre est physiquement absent
    }

    // Fait clignoter la LED verte GP25 pour confirmer l'acquisition active
    gpio_put(PIN_LED_STATUS, !gpio_get(PIN_LED_STATUS));

    // 1. Lecture de la pression actuelle
    float press = lps22hb_read_pressure();

    // 2. Calibration de la pression de référence au sol (PRE_FLIGHT)
    if (currentState == PRE_FLIGHT) {
        if (ground_pressure < 0.0f) {
            ground_pressure = press; // Initialisation de départ
        } else {
            // Lissage lent par filtre passe-bas exponentiel
            ground_pressure = (ground_pressure * 0.98f) + (press * 0.02f);
        }
        return; // On ne fait pas tourner le filtre de Kalman au sol
    }

    // Si la pression de sol est invalide, impossible de continuer
    if (ground_pressure < 0.0f) {
        return;
    }

    // 3. Calcul de l'altitude mesurée
    float z_meas = lps22hb_hpa_to_altitude(press, ground_pressure);

    // 4. Étape de prédiction du filtre de Kalman 1D (dt = 40 ms)
    float dt = 0.040f;
    float z_pred = kalman_z + (kalman_v * dt);
    float v_pred = kalman_v;

    P_cov[0][0] = P_cov[0][0] + dt * (P_cov[1][0] + P_cov[0][1]) + dt * dt * P_cov[1][1] + Q_alt;
    P_cov[0][1] = P_cov[0][1] + dt * P_cov[1][1];
    P_cov[1][0] = P_cov[0][1];
    P_cov[1][1] = P_cov[1][1] + Q_vel;

    // 5. Étape de correction/mise à jour du filtre de Kalman 1D
    float S = P_cov[0][0] + R_alt;
    float K0 = P_cov[0][0] / S;
    float K1 = P_cov[1][0] / S;

    float y = z_meas - z_pred; // Résidu d'innovation

    kalman_z = z_pred + (K0 * y);
    kalman_v = v_pred + (K1 * y);

    P_cov[0][0] = (1.0f - K0) * P_cov[0][0];
    P_cov[0][1] = (1.0f - K0) * P_cov[0][1];
    P_cov[1][0] = P_cov[0][1];
    P_cov[1][1] = P_cov[1][1] - K1 * P_cov[0][1];

    // 6. Algorithme de détection d'apogée (en vol uniquement)
    if (currentState == ASCEND || currentState == WINDOW) {
        // Condition : altitude > 15m (marge de sécurité) et vitesse estimée négative (redescente)
        if (kalman_z > 15.0f && kalman_v < -1.0f) {
            apogee_counter++;
            if (apogee_counter >= 5) { // 5 mesures consécutives (~200 ms) pour confirmer
                if (triggerBaroApogee == 0) {
                    triggerBaroApogee = 1;
                    log_entryf("[KALMAN] Apogee detectee ! Alt = %.2fm, Vel = %.2fm/s", kalman_z, kalman_v);
                    debug_printf("[KALMAN] Apogee detectee ! Alt = %.2f m, Vel = %.2f m/s\n", kalman_z, kalman_v);
                }
            }
        } else {
            apogee_counter = 0; // Réinitialise en cas de remontée ou d'altitude insuffisante
        }
    }
}

