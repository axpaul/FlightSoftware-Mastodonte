// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : display.cpp
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Gestion de l'écran OLED SSD1306 (I2C0 sur GP8/GP9)
// -------------------------------------------------------------

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pico/unique_id.h"

#include "display.h"
#include "platform.h"
#include "sequencer.h"
#include "lps22hb.h"
#include "lsm6dsl.h"
#include "system.h"
#include "telemetry.h"
#include "log.h"
#include "debug.h"
#include <math.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static bool display_present = false;

// Conversion des états FSM en chaînes anglaises
static const char* state_to_string(rocket_state_t state) {
    switch (state) {
        case PRE_FLIGHT:   return "PRE-FLIGHT";
        case PYRO_RDY:     return "ARMED / READY";
        case ASCEND:       return "ASCEND";
        case WINDOW:       return "WINDOW";
        case DEPLOY_ALGO:  // fallthrough
        case DEPLOY_TIMER: return "DEPLOYMENT";
        case DESCEND:      return "DESCEND";
        case TOUCHDOWN:    return "TOUCHDOWN";
        case ERROR_SEQ:    return "ERR-SEQ";
        case ERROR_MOTOR:  return "ERR-MOTOR";
        default:           return "UNKNOWN";
    }
}

bool display_init(void) {
    log_entry("[OLED] Initialising I2C0 (GP8=SDA, GP9=SCL) at 400 kHz");
    debug_println("[OLED] Initialising I2C0 (GP8=SDA, GP9=SCL) at 400 kHz...");

    // Initialisation du bus I2C0 sur les pins GP8 (SDA) et GP9 (SCL)
    Wire.setSDA(PIN_I2C0_SDA);
    Wire.setSCL(PIN_I2C0_SCL);
    Wire.begin();

    // Tente d'initialiser l'écran SSD1306 à l'adresse 0x3C
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        log_entry("[OLED] ERROR: SSD1306 OLED screen not detected on 0x3C. Security bypass.");
        debug_println("[OLED] ERROR: SSD1306 OLED screen not detected! Running headless.");
        display_present = false;
        return false;
    }

    display_present = true;

    // Configuration par défaut de l'écran
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.display();

    log_entry("[OLED] SSD1306 OLED screen detected and initialized successfully.");

    // Récupération de l'identifiant unique du RP2040
    char id_string[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id_string, sizeof(id_string));

    // --- Animation du Démarrage (Boot Animation) ---
    // Affiche le nom "MASTODONTE", la version et l'ID unique avec une fusée réaliste horizontale traversant le titre
    for (int16_t frame = 0; frame <= 60; frame++) {
        display.clearDisplay();

        // Titre principal "MASTODONTE"
        display.setTextSize(2);
        display.setCursor(4, 12);
        display.print("MASTODONTE");

        // Version en bas (centrée)
        display.setTextSize(1);
        display.setCursor(34, 38);
        display.print("FW: ");
        display.print(FW_VERSION);

        // ID Unique du RP2040 tout en bas (centré)
        display.setCursor(4, 52);
        display.print("ID: ");
        display.print(id_string);

        // Coordonnées de la fusée (vol horizontal traversant le texte à ry = 20)
        int16_t rx = -30 + (180 * frame / 60);
        int16_t ry = 20;

        // --- Dessin d'une fusée réaliste (horizontale, pointée vers la droite) ---
        // Corps de la fusée (cylindre épais tracé par lignes parallèles et remplissage)
        display.drawFastHLine(rx, ry - 2, 20, SSD1306_WHITE);
        display.drawFastHLine(rx, ry + 2, 20, SSD1306_WHITE);
        display.fillRect(rx,      ry - 1, 20, 3, SSD1306_WHITE);

        // Ligne noire de séparation des étages
        display.drawFastVLine(rx + 8, ry - 2, 5, SSD1306_BLACK);

        // Cône de nez (ogive pointue à l'avant, pointant vers la droite)
        display.fillTriangle(rx + 20, ry - 2, rx + 20, ry + 2, rx + 26, ry, SSD1306_WHITE);

        // Ailerons arrière stabilisateurs (fins profilées sur les côtés)
        display.fillTriangle(rx, ry - 2, rx - 4, ry - 6, rx + 4, ry - 2, SSD1306_WHITE); // Aileron supérieur
        display.fillTriangle(rx, ry + 2, rx - 4, ry + 6, rx + 4, ry + 2, SSD1306_WHITE); // Aileron inférieur
        display.drawLine(rx,     ry,     rx - 3, ry,     SSD1306_WHITE);                 // Aileron central / tuyère

        // Propulseur (tuyère arrière)
        display.fillRect(rx - 2, ry - 1, 2, 3, SSD1306_WHITE);

        // Effet de flamme de poussée vacillante (pointant vers la gauche)
        int16_t flame_len = (frame % 2 == 0) ? 8 : 15;
        // Flamme centrale
        display.drawLine(rx - 3, ry, rx - 3 - flame_len, ry, SSD1306_WHITE);
        // Flammes latérales (plus courtes)
        display.drawLine(rx - 3, ry - 1, rx - 3 - (flame_len * 2 / 3), ry - 1, SSD1306_WHITE);
        display.drawLine(rx - 3, ry + 1, rx - 3 - (flame_len * 2 / 3), ry + 1, SSD1306_WHITE);

        display.display();
        delay(25); // ~40 FPS, animation sur 1.5 secondes
    }

    delay(800); // Pause pour laisser le temps de lire l'ID unique
    return true;
}

void display_update(void) {
    if (!display_present) {
        return; // Évite les appels I2C si l'écran n'est pas branché physiquement
    }



    // 2. Limitation du taux de rafraîchissement au sol (5 Hz pour ne pas surcharger la boucle principale)
    static uint32_t last_refresh = 0;
    if (millis() - last_refresh < 200) {
        return;
    }
    last_refresh = millis();

    display.clearDisplay();

    // 3. Dessin de l'en-tête permanent (Header)
    float volt = battery_read_voltage();
    bool usb_active = Serial; // true si port COM connecté et ouvert

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Batterie à gauche
    display.setCursor(0, 0);
    display.print("BATT: ");
    display.print(volt, 2);
    display.print("V");

    // USB à droite
    display.setCursor(82, 0);
    if (usb_active) {
        display.print("USB: OK");
    } else {
        display.print("USB: --");
    }

    // Ligne de séparation sous l'en-tête (y = 10)
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    // 4. Rendu du contenu selon l'état actuel
    if (currentState == TOUCHDOWN) {
        // --- PAGE DE RAPPORT DE VOL (TOUCHDOWN) ---
        display.setTextSize(1);
        display.setCursor(16, 14);
        display.print("** FLIGHT REPORT **");

        float max_alt = lps22hb_get_max_altitude();
        float max_vel = telemetry_get_max_velocity();
        float max_acc = telemetry_get_max_acceleration();

        display.setCursor(0, 26);
        display.print("Max Alt  : "); display.print(max_alt, 1); display.print(" m");

        display.setCursor(0, 35);
        display.print("Max Vel  : "); display.print(max_vel, 1); display.print(" m/s");

        display.setCursor(0, 44);
        display.print("Max Accel: "); display.print(max_acc, 2); display.print(" g");

        display.setCursor(0, 53);
        if (telemetry_is_apogee_recorded()) {
            display.print("Apogee   : T+"); 
            display.print((float)telemetry_get_apogee_time_ms() / 1000.0f, 1);
            display.print(" s");
        } else {
            display.print("Apogee   : Not detected");
        }
    } else {
        // --- PAGES DIAGNOSTIQUES DE PRE-VOL (Alternance toutes les 3s) ---
        static uint32_t last_switch = 0;
        static uint8_t current_page = 0;

        if (millis() - last_switch > 3000) {
            current_page = (current_page + 1) % 2;
            last_switch = millis();
        }

        uint8_t jack = gpio_get(PIN_SMITCH_N1);
        uint8_t rbf  = gpio_get(PIN_SMITCH_N2);

        if (current_page == 0) {
            // Page 1 : Statut du Séquenceur
            display.setCursor(0, 14);
            display.print("STATE: "); 
            display.print(state_to_string(currentState));

            display.setCursor(0, 26);
            display.print("RBF  : "); 
            display.print(rbf == 0 ? "REMOVED" : "INSERTED");

            display.setCursor(0, 36);
            display.print("JACK : "); 
            display.print(jack == 0 ? "REMOVED" : "CONNECTED");

            display.setCursor(0, 46);
            display.print("OPTO3: "); 
            display.print(gpio_get(PIN_OCTO_N3) ? "HIGH" : "LOW");

            display.setCursor(0, 56);
            display.print("OPTO4: "); 
            display.print(gpio_get(PIN_OCTO_N4) ? "HIGH" : "LOW");
        } else {
            // Page 2 : Capteurs & Kalman (Pas d'I2C actif sur i2c1 pour decorreler de la sequence)
            float press = lps22hb_get_last_pressure();
            float alt = lps22hb_get_kalman_altitude();
            float vel = lps22hb_get_kalman_velocity();
            
            float ax = 0.0f, ay = 0.0f, az = 0.0f;
            lsm6dsl_get_last_accel(&ax, &ay, &az);
            float acc_mag = sqrtf(ax*ax + ay*ay + az*az);

            display.setCursor(0, 14);
            display.print("PRES : "); display.print(press, 1); display.print(" hPa");

            display.setCursor(0, 26);
            display.print("ALT  : "); display.print(alt, 1); display.print(" m");

            display.setCursor(0, 36);
            display.print("VEL  : "); display.print(vel, 1); display.print(" m/s");

            display.setCursor(0, 46);
            display.print("ACC  : "); display.print(acc_mag, 2); display.print(" g");

            display.setCursor(0, 56);
            display.print("TEMP : "); display.print(temperature_read_mcu(), 1); display.print(" C");
        }
    }

    display.display();
}
