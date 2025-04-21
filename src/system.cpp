// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "system.h"

Adafruit_NeoPixel rgb(1, PIN_LED_WS2812, NEO_GRB + NEO_KHZ800);

timestamp_t compute_timestamp(absolute_time_t timestamp) {
    int64_t total_us = to_us_since_boot(timestamp);
  
    timestamp_t result;
    result.minutes      = total_us / 60000000;
    result.seconds      = (total_us % 60000000) / 1000000;
    result.milliseconds = (total_us % 1000000) / 1000;
    result.microseconds = total_us % 1000;
  
    return result;
  }

float battery_read_voltage(void) {
  adc_select_input(2); // GPIO28
  uint16_t raw = adc_read();

  float vRef = 3.3f;                         // Référence ADC
  float adcVoltage = (raw / 4095.0f) * vRef; // Tension lue
  float batteryVoltage = (adcVoltage * 6.0f);  // Pont diviseur (ex: R1=1k, R2=200Ω)

  return batteryVoltage;
}

float temperature_read_mcu(void) {
  adc_set_temp_sensor_enabled(true);
  adc_select_input(4); // Température interne

  uint16_t raw = adc_read();
  float vRef = 3.3f;
  float adcVoltage = (raw / 4095.0f) * vRef;

  float temperature = 27.0f - (adcVoltage - 0.706f) / 0.001721f;
  return temperature;
}
  
void setup_pin(void){
  
  //pinMode(PIN_VCC_BAT, INPUT);

  adc_gpio_init(28);       // Active la fonction ADC sur GPIO28
  adc_select_input(2);     // Sélectionne l'entrée ADC2

  gpio_init(PIN_LED_STATUS);
  gpio_init(PIN_SMITCH_N1);
  gpio_init(PIN_SMITCH_N2);
  gpio_init(PIN_OCTO_N3);
  gpio_init(PIN_OCTO_N4);
  gpio_init(IN1_M1);
  gpio_init(IN2_M1);
  gpio_init(IN1_M2);
  gpio_init(IN2_M2);
  gpio_init(IN1_M3);
  gpio_init(IN2_M3);
  
  gpio_set_dir(PIN_LED_STATUS, GPIO_OUT);
  gpio_set_dir(PIN_SMITCH_N1, GPIO_IN);
  gpio_set_dir(PIN_SMITCH_N2, GPIO_IN);
  gpio_set_dir(PIN_OCTO_N3, GPIO_IN);
  gpio_set_dir(PIN_OCTO_N4, GPIO_IN);
  gpio_set_dir(IN1_M1, GPIO_OUT);
  gpio_set_dir(IN2_M1, GPIO_OUT);
  gpio_set_dir(IN1_M2, GPIO_OUT);
  gpio_set_dir(IN2_M2, GPIO_OUT);
  gpio_set_dir(IN1_M3, GPIO_OUT);
  gpio_set_dir(IN2_M3, GPIO_OUT);

  gpio_disable_pulls(PIN_LED_STATUS);   
  gpio_disable_pulls(PIN_SMITCH_N1);     
  gpio_disable_pulls(PIN_SMITCH_N1);  
  gpio_disable_pulls(PIN_OCTO_N3);    
  gpio_disable_pulls(PIN_OCTO_N4);  
  gpio_disable_pulls(IN1_M1);
  gpio_disable_pulls(IN2_M1);
  gpio_disable_pulls(IN1_M2);
  gpio_disable_pulls(IN2_M2);
  gpio_disable_pulls(IN1_M3);
  gpio_disable_pulls(IN2_M3);

}


void setup_rgb(void){
  rgb.begin();
  rgb.setBrightness(50);
  rgb.clear();
  rgb.setPixelColor(0, rgb.Color(0, 255, 0));
  rgb.show();
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
          setBuzzer(true, 30000, 5000, freqL);      // Bip long toutes les 30 secondes
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
