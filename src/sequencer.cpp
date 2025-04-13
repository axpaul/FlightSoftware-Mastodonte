// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "sequencer.h"

rocket_state_t currentState = PRE_FLIGHT;
volatile uint8_t triggerRBF = 0;

void seq_arm_rbf(){
    timestamp_t ts = compute_timestamp(get_absolute_time());
    debug_println("[INTERRUPT] Detect : ");
    debug_printf("[INTERRUPT] T = %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[INTERRUPT] Pin value : %d\n", digitalRead(PIN_SMITCH_N2));
    
    if (digitalRead(PIN_SMITCH_N2) == 0){
      triggerRBF = 1;
      debug_println("[INTERRUPT] High");
    }
      
    if (digitalRead(PIN_SMITCH_N2) == 1){
      triggerRBF = 2;
      debug_println("[INTERRUPT] Low");
    }
}

rocket_state_t seq_handle(void) {
    switch (currentState) {
        case PRE_FLIGHT:
            currentState = seq_preLaunch();
            break;
        case PYRO_RDY:
            currentState = seq_pyroRdy();
            break;
        // etc.
    }
    return currentState;
}

void apply_state_config(rocket_state_t state) {
  uint32_t color = 0;
  uint16_t freq = 2000;   // Fréquence standard du buzzer
  uint16_t freqL = 300;   // Fréquence "low", plus grave (pour feedback discret)

  switch (state) {

      // === État : Pré-vol ===
      case PRE_FLIGHT:
          color = rgb.Color(0, 255, 0);             // 🟢 Vert : prêt au sol
          setBuzzer(true, 3000, 200, freqL);        // Double bip très espacé, discret
          break;

      // === État : Pyro prêt ===
      case PYRO_RDY:
          color = rgb.Color(255, 255, 0);           // 🟡 Jaune : charge armée
          setBuzzer(true, 1000, 500, freqL);        // Bip lent, 1 bip/sec
          break;

      // === État : Ascension ===
      case ASCEND:
          color = rgb.Color(0, 0, 255);             // 🔵 Bleu : en montée
          setBuzzer(true, 100, 75, freqL);          // Bip ultra rapide
          break;

      // === État : Fenêtre d’ouverture (timer ou algo) ===
      case WINDOW:
          color = rgb.Color(0, 255, 255);           // 🟦 Cyan : fenêtre active
          setBuzzer(true, 400, 300, freqL);         // Bip rapide (alerte)
          break;

      // === État : Déploiement par algo ou timer ===
      case DEPLOY_ALGO:
      case DEPLOY_TIMER:
          color = rgb.Color(255, 165, 0);           // 🟠 Orange : déploiement
          setBuzzer(true, 400, 400, freqL);         // Bip continu mais intermittent
          break;

      // === État : Descente sous parachute ===
      case DESCEND:
          color = rgb.Color(255, 0, 255);           // 🟣 Magenta : descente
          setBuzzer(true, 1000, 200, freqL);        // Bip lent et régulier
          break;

      // === État : Atterrissage ===
      case TOUCHDOWN:
          color = rgb.Color(0, 255, 0);             // 🟢 Vert fixe : au sol, OK
          setBuzzer(true, 60000, 5000, freqL);      // Bip long toutes les 60 secondes
          break;

      // === État : Erreur système / séquence ===
      case ERROR_SEQ:
          color = rgb.Color(255, 0, 0);             // 🔴 Rouge : erreur
          setBuzzer(true, 500, 250, 500);           // Bip rapide et aigu
          break;
  }

  rgb.setPixelColor(0, color);
  rgb.show();
}
