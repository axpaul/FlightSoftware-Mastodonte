// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : test_main.cpp
// # Author  : Paul Miailhe
// # Date    : 2026-06-16
// # Object  : Tests unitaires du logiciel de vol sur RP2040
// -------------------------------------------------------------

#include <Arduino.h>
#include <unity.h>

#include "platform.h"
#include "sequencer.h"
#include "system.h"
#include "lps22hb.h"

// Variables globales externes pour simuler et vérifier
extern rocket_state_t currentState;
extern volatile uint8_t triggerRBF;
extern volatile uint8_t triggerJack;
extern volatile uint8_t triggerTouch;
extern volatile uint8_t triggerBaroApogee;

void setUp(void) {
    // S'exécute avant chaque test
    seq_reset_triggers();
    currentState = PRE_FLIGHT;
}

void tearDown(void) {
    // S'exécute après chaque test
}

// ==========================================
// 1. TESTS DU SEQUENCEUR (FSM)
// ==========================================

void test_fsm_initial_state(void) {
    TEST_ASSERT_EQUAL(PRE_FLIGHT, currentState);
}

void test_fsm_transition_rbf_remove(void) {
    // Simulation du retrait RBF (triggerRBF = 1) dans PRE_FLIGHT
    triggerRBF = 1;
    rocket_state_t nextState = seq_handle();
    
    TEST_ASSERT_EQUAL(PYRO_RDY, nextState);
    TEST_ASSERT_EQUAL(PYRO_RDY, currentState);
    TEST_ASSERT_EQUAL(0, triggerRBF); // Doit être réinitialisé
}

void test_fsm_transition_rbf_insert(void) {
    // Passage manuel dans l'état PYRO_RDY
    currentState = PYRO_RDY;
    
    // Simulation de l'insertion RBF (triggerRBF = 2) dans PYRO_RDY
    triggerRBF = 2;
    rocket_state_t nextState = seq_handle();
    
    TEST_ASSERT_EQUAL(PRE_FLIGHT, nextState);
    TEST_ASSERT_EQUAL(PRE_FLIGHT, currentState);
    TEST_ASSERT_EQUAL(0, triggerRBF); // Doit être réinitialisé
}

void test_fsm_transition_jack_remove(void) {
    // Passage manuel dans l'état PYRO_RDY
    currentState = PYRO_RDY;
    
    // Simulation du retrait Jack (triggerJack = 1)
    triggerJack = 1;
    rocket_state_t nextState = seq_handle();
    
    TEST_ASSERT_EQUAL(ASCEND, nextState);
    TEST_ASSERT_EQUAL(ASCEND, currentState);
    TEST_ASSERT_EQUAL(0, triggerJack); // Doit être réinitialisé
}

// ==========================================
// 2. TESTS DE CALCUL ET FILTRAGE (KALMAN / BARO)
// ==========================================

void test_baro_pressure_to_altitude(void) {
    // Test 1: Pression actuelle égale à la pression au sol -> Altitude = 0m
    float alt0 = lps22hb_hpa_to_altitude(1013.25f, 1013.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, alt0);
    
    // Test 2: Pression actuelle plus basse -> Altitude positive
    float alt_pos = lps22hb_hpa_to_altitude(900.0f, 1013.25f);
    TEST_ASSERT_TRUE(alt_pos > 0.0f);
    // 900 hPa correspond à environ 988 mètres d'altitude
    TEST_ASSERT_FLOAT_WITHIN(10.0f, 988.0f, alt_pos);
    
    // Test 3: Pression au sol invalide (<= 0) -> Retourne 0m
    float alt_inv = lps22hb_hpa_to_altitude(900.0f, -10.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, alt_inv);
}

void test_kalman_reset(void) {
    // On force des valeurs arbitraires via les setters de test
    lps22hb_set_kalman_state(150.0f, 25.0f);
    
    // Réinitialisation
    lps22hb_reset_kalman();
    
    // Vérification que les états sont à zéro
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, lps22hb_get_kalman_altitude());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, lps22hb_get_kalman_velocity());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, lps22hb_get_max_altitude());
}

void test_kalman_equations_math(void) {
    lps22hb_reset_kalman();
    lps22hb_set_ground_pressure(1013.25f);
    lps22hb_set_baro_present(true);
    
    // On ne peut pas facilement appeler lps22hb_update_kalman() sans I2C physique,
    // mais on peut tester les setters d'injection d'état de test.
    lps22hb_set_kalman_state(12.5f, 1.8f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, lps22hb_get_kalman_altitude());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.8f, lps22hb_get_kalman_velocity());
}

void test_kalman_apogee_detection(void) {
    lps22hb_reset_kalman();
    currentState = ASCEND;
    triggerBaroApogee = 0;

    // Simulation de 4 étapes avec z = 20m (z > 15m) et v = -2.0m/s (v < -1m/s)
    lps22hb_set_kalman_state(20.0f, -2.0f);
    for (int i = 0; i < 4; ++i) {
        lps22hb_test_apogee_detection_step();
        TEST_ASSERT_EQUAL(0, triggerBaroApogee); // Pas encore déclenché
    }

    // 5ème étape -> Doit se déclencher
    lps22hb_test_apogee_detection_step();
    TEST_ASSERT_EQUAL(1, triggerBaroApogee);

    // Si on réinitialise et qu'il y a une remontée, le compteur doit se remettre à zéro
    lps22hb_reset_kalman();
    triggerBaroApogee = 0;
    
    // Étape 1 : descente
    lps22hb_set_kalman_state(20.0f, -2.0f);
    lps22hb_test_apogee_detection_step();
    // Étape 2 : remontée (v > -1.0f)
    lps22hb_set_kalman_state(21.0f, 0.5f);
    lps22hb_test_apogee_detection_step();
    
    // Re-descente pendant 4 cycles
    lps22hb_set_kalman_state(20.0f, -2.0f);
    for (int i = 0; i < 4; ++i) {
        lps22hb_test_apogee_detection_step();
        TEST_ASSERT_EQUAL(0, triggerBaroApogee);
    }
    // 5ème cycle -> Apogée
    lps22hb_test_apogee_detection_step();
    TEST_ASSERT_EQUAL(1, triggerBaroApogee);
}


// ==========================================
// 3. TESTS SYSTEME (ADC / TIMESTAMPS)
// ==========================================

void test_system_timestamp_computation(void) {
    // 0 microseconde
    absolute_time_t t0 = from_us_since_boot(0);
    timestamp_t ts0 = compute_timestamp(t0);
    TEST_ASSERT_EQUAL(0, ts0.minutes);
    TEST_ASSERT_EQUAL(0, ts0.seconds);
    TEST_ASSERT_EQUAL(0, ts0.milliseconds);
    TEST_ASSERT_EQUAL(0, ts0.microseconds);
    
    // 1 minute, 2 secondes, 345 millisecondes, 678 microsecondes
    // = 60 * 1e6 + 2 * 1e6 + 345 * 1000 + 678 = 62,345,678 us
    absolute_time_t t1 = from_us_since_boot(62345678ULL);
    timestamp_t ts1 = compute_timestamp(t1);
    TEST_ASSERT_EQUAL(1, ts1.minutes);
    TEST_ASSERT_EQUAL(2, ts1.seconds);
    TEST_ASSERT_EQUAL(345, ts1.milliseconds);
    TEST_ASSERT_EQUAL(678, ts1.microseconds);
}

void test_system_battery_read_bounds(void) {
    // Initialisation ADC au cas où
    adc_init();
    adc_gpio_init(28);
    
    float batt = battery_read_voltage();
    // La tension de batterie doit être positive et dans une plage réaliste pour le montage
    // (par exemple, 0.0V à 30.0V)
    TEST_ASSERT_TRUE(batt >= 0.0f);
    TEST_ASSERT_TRUE(batt <= 30.0f);
}

void test_system_mcu_temperature_bounds(void) {
    float temp = temperature_read_mcu();
    // La température interne du MCU doit être dans une plage de fonctionnement réaliste
    // (par exemple, -10°C à 85°C)
    TEST_ASSERT_TRUE(temp >= -10.0f);
    TEST_ASSERT_TRUE(temp <= 85.0f);
}

// ==========================================
// POINT D'ENTREE DES TESTS UNITAIRES
// ==========================================

void setup() {
    // Attente pour laisser le temps à la liaison USB série de s'établir
    delay(2000);

    UNITY_BEGIN();
    
    // Exécution des tests FSM
    RUN_TEST(test_fsm_initial_state);
    RUN_TEST(test_fsm_transition_rbf_remove);
    RUN_TEST(test_fsm_transition_rbf_insert);
    RUN_TEST(test_fsm_transition_jack_remove);
    
    // Exécution des tests Kalman / Altitude
    RUN_TEST(test_baro_pressure_to_altitude);
    RUN_TEST(test_kalman_reset);
    RUN_TEST(test_kalman_equations_math);
    RUN_TEST(test_kalman_apogee_detection);

    
    // Exécution des tests Système
    RUN_TEST(test_system_timestamp_computation);
    RUN_TEST(test_system_battery_read_bounds);
    RUN_TEST(test_system_mcu_temperature_bounds);
    
    UNITY_END();
}

void loop() {
    // Rien à faire dans la boucle de test
}
