// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "system.h"
#include "log.h"
#include "hardware/i2c.h"

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
  gpio_init(NFAUT_M1);            
  gpio_init(NFAUT_M2);         
  gpio_init(NFAUT_M3);
  gpio_init(24);
    
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
  gpio_set_dir(NFAUT_M1, GPIO_IN);
  gpio_set_dir(NFAUT_M2, GPIO_IN);
  gpio_set_dir(NFAUT_M3, GPIO_IN);
  gpio_set_dir(24, GPIO_IN);

  gpio_disable_pulls(PIN_LED_STATUS);   
  gpio_disable_pulls(PIN_SMITCH_N1);     
  gpio_disable_pulls(PIN_SMITCH_N2);  
  gpio_disable_pulls(PIN_OCTO_N3);    
  gpio_disable_pulls(PIN_OCTO_N4);  
  gpio_disable_pulls(IN1_M1);
  gpio_disable_pulls(IN2_M1);
  gpio_disable_pulls(IN1_M2);
  gpio_disable_pulls(IN2_M2);
  gpio_disable_pulls(IN1_M3);
  gpio_disable_pulls(IN2_M3);
  gpio_pull_up(24);
  

  // Active les pull-up internes sur les broches nFAULT des DRV8872
  // Ces broches sont en open-drain : sans pull-up, elles peuvent flotter
  // Cela permet une lecture fiable de l'état de défaut (LOW = fault actif, HIGH = normal)
  gpio_pull_up(NFAUT_M1);
  gpio_pull_up(NFAUT_M2);
  gpio_pull_up(NFAUT_M3);
}

void system_i2c_init(void) {
  log_entry("[SYSTEM] Initialisation I2C1 (GP6=SDA, GP7=SCL) a 400 kHz");
  debug_println("[SYSTEM] Initialisation I2C1 (GP6=SDA, GP7=SCL) a 400 kHz...");

  // Initialise i2c1 à 400 kHz
  i2c_init(i2c1, 400000);
  
  // Configure les broches pour la fonction I2C
  gpio_set_function(PIN_I2C1_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_I2C1_SCL, GPIO_FUNC_I2C);
  
  // Active les pull-ups internes
  gpio_pull_up(PIN_I2C1_SDA);
  gpio_pull_up(PIN_I2C1_SCL);
}


void setup_rgb(void){
  rgb.begin();
  rgb.setBrightness(50);
  rgb.clear();
  rgb.setPixelColor(0, rgb.Color(0, 255, 0));
  rgb.show();
}

// --- Variables internes pour la surveillance batterie ---
volatile uint8_t trigger_battery_check = 0;
static struct repeating_timer battery_timer;
static bool battery_is_low = false;
static bool blink_state = false;

// Callback déclenchée par le timer matériel toutes les secondes (contexte ISR)
bool battery_timer_callback(struct repeating_timer *t) {
  trigger_battery_check = 1;
  return true; // Demande de répéter le timer
}

// Initialisation du monitoring de batterie
void system_battery_monitor_init(void) {
  add_repeating_timer_ms(1000, battery_timer_callback, NULL, &battery_timer);
}

// Table de correspondance des couleurs de la LED selon l'état
uint32_t get_state_color(rocket_state_t state) {
  switch (state) {
      case PRE_FLIGHT:   return rgb.Color(0, 255, 0);     // Vert : prêt au sol
      case PYRO_RDY:     return rgb.Color(255, 255, 0);   // Jaune : Moteur prêt
      case ASCEND:       return rgb.Color(0, 0, 255);     // Bleu : en montée
      case WINDOW:       return rgb.Color(0, 255, 255);   // Cyan : fenêtre active
      case DEPLOY_ALGO:
      case DEPLOY_TIMER: return rgb.Color(255, 165, 0);   // Orange : déploiement
      case DESCEND:      return rgb.Color(255, 0, 255);   // Magenta : descente
      case TOUCHDOWN:    return rgb.Color(0, 255, 0);     // Vert fixe : au sol, OK
      case ERROR_SEQ:    
      case ERROR_MOTOR:
      default:           return rgb.Color(255, 0, 0);     // Rouge : erreur
  }
}

// Fonction de tick appelée dans la boucle principale
void system_battery_check_tick(void) {
  if (trigger_battery_check == 1) {
    trigger_battery_check = 0; // Acquittement du drapeau
    
    float voltage = battery_read_voltage();
    bool currently_low = (voltage <= 6.0f);
    
    // Si l'état de la batterie change
    if (currently_low != battery_is_low) {
      battery_is_low = currently_low;
      
      // Si la batterie redevient OK, on remet immédiatement la couleur fixe de l'état actuel
      if (!battery_is_low) {
        rgb.setPixelColor(0, get_state_color(currentState));
        rgb.show();
        log_entryf("[BATTERY] Tension retablie : %.2f V", voltage);
        debug_printf("[BATTERY] Tension retablie : %.2f V\n", voltage);
      } else {
        // En cas de sous-tension détectée
        log_entryf("[BATTERY] ALERTE : Sous-tension critique : %.2f V", voltage);
        debug_printf("[BATTERY] ALERTE : Sous-tension critique : %.2f V !\n", voltage);
      }
    }
    
    // Si la batterie est basse, on applique l'alternance LED
    if (battery_is_low) {
      if (blink_state) {
        rgb.setPixelColor(0, rgb.Color(255, 0, 0)); // Rouge (Alerte)
      } else {
        rgb.setPixelColor(0, get_state_color(currentState)); // Couleur de l'état actuel
      }
      rgb.show();
      blink_state = !blink_state;
    }
  }
}

void apply_state_config(rocket_state_t state) {
  uint32_t color = get_state_color(state);
  //uint16_t freq = 2000;   // Fréquence standard du buzzer
  uint16_t freq = 300;   // Fréquence "low", plus grave (pour feedback discret)

  switch (state) {

      // === État : Pré-vol ===
      case PRE_FLIGHT:
          setBuzzer(true, 3000, 200, freq);        // Double bip très espacé, discret
          break;

      // === État : Pyro prêt ===
      case PYRO_RDY:
          setBuzzer(true, 1000, 500, freq);        // Bip lent, 1 bip/sec
          break;

      // === État : Ascension ===
      case ASCEND:
          setBuzzer(true, 100, 75, freq);          // Bip ultra rapide
          break;

      // === État : Fenêtre d’ouverture (timer ou algo) ===
      case WINDOW:
          setBuzzer(true, 400, 300, freq);         // Bip rapide (alerte)
          break;

      // === État : Déploiement par algo ou timer ===
      case DEPLOY_ALGO:
      case DEPLOY_TIMER:
          setBuzzer(true, 400, 400, freq);         // Bip continu mais intermittent
          break;

      // === État : Descente sous parachute ===
      case DESCEND:
          setBuzzer(true, 1000, 200, freq);        // Bip lent et régulier
          break;

      // === État : Atterrissage ===
      case TOUCHDOWN:
          setBuzzer(true, 30000, 5000, freq);      // Bip long toutes les 30 secondes
          break;

      // === État : Erreur système / séquence ===
      case ERROR_SEQ:
          setBuzzer(true, 500, 250, 500);           // Bip rapide et aigu
          break;

      // === État : Erreur moteur ===
      case ERROR_MOTOR:
          setBuzzer(true, 300, 150, 600);           // Bip très rapide
          break;
  }

  // Si la batterie n'est pas faible, on applique la couleur immédiatement.
  if (!battery_is_low) {
    rgb.setPixelColor(0, color);
    rgb.show();
  }
}

void low_power_delay_ms(uint32_t total_ms) {
  const uint32_t step_ms = 10; // Pas de 10 ms
  uint32_t remaining = total_ms;

  while (remaining > 0) {
      __wfi();                         // Attend une interruption (low power)
      sleep_ms(step_ms);              // Retarde légèrement pour temporiser
      remaining = (remaining > step_ms) ? (remaining - step_ms) : 0;
  }
}