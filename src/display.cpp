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
static bool screen_turned_off = false;

// Conversion des états FSM en chaînes françaises
static const char* state_to_string(rocket_state_t state) {
    switch (state) {
        case PRE_FLIGHT:   return "PRE-VOL";
        case PYRO_RDY:     return "ARME / PRET";
        case ASCEND:       return "MONTEE";
        case WINDOW:       return "FENETRE";
        case DEPLOY_ALGO:  // fallthrough
        case DEPLOY_TIMER: return "DEPLOIEMENT";
        case DESCEND:      return "DESCENTE";
        case TOUCHDOWN:    return "ATTERRI";
        case ERROR_SEQ:    return "ERR-SEQ";
        case ERROR_MOTOR:  return "ERR-MOTEUR";
        default:           return "INCONNU";
    }
}

bool display_init(void) {
    log_entry("[OLED] Initialisation I2C0 (GP8=SDA, GP9=SCL) a 400 kHz");
    debug_println("[OLED] Initialisation I2C0 (GP8=SDA, GP9=SCL) a 400 kHz...");

    // Initialisation du bus I2C0 sur les pins GP8 (SDA) et GP9 (SCL)
    Wire.setSDA(PIN_I2C0_SDA);
    Wire.setSCL(PIN_I2C0_SCL);
    Wire.begin();

    // Tente d'initialiser l'écran SSD1306 à l'adresse 0x3C
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        log_entry("[OLED] ERROR: Ecran SSD1306 non detecte sur l'adresse 0x3C. Bypass de securite.");
        debug_println("[OLED] ERROR: Ecran SSD1306 non detecte ! Fonctionnement sans ecran active.");
        display_present = false;
        return false;
    }

    display_present = true;
    screen_turned_off = false;

    // Configuration par défaut de l'écran
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.display();

    log_entry("[OLED] Ecran SSD1306 detecte et initialise avec succes.");

    // --- Animation du Démarrage (Boot Animation) ---
    // Affiche le nom "MASTODONTE" avec une fusée en diagonale
    for (int16_t frame = 0; frame <= 50; frame++) {
        display.clearDisplay();

        // Texte principal "MASTODONTE" au centre
        display.setTextSize(2);
        display.setCursor(4, 24);
        display.print("MASTODONTE");

        // Calcul des coordonnées du missile (diagonale de bas-gauche à haut-droite)
        int16_t rx = -20 + (160 * frame / 50);
        int16_t ry = 70 - (90 * frame / 50);

        // Dessin du missile (pointé à 45 degrés vers le haut-droite)
        display.drawLine(rx, ry, rx + 10, ry - 10, SSD1306_WHITE); // Corps
        display.fillTriangle(rx + 10, ry - 10, rx + 13, ry - 13, rx + 12, ry - 8, SSD1306_WHITE); // Nez cone
        display.fillTriangle(rx + 10, ry - 10, rx + 13, ry - 13, rx + 8, ry - 12, SSD1306_WHITE); // Nez cone
        display.drawLine(rx, ry, rx - 3, ry + 1, SSD1306_WHITE); // Aileron
        display.drawLine(rx, ry, rx + 1, ry + 3, SSD1306_WHITE); // Aileron

        // Effet flamme vacillante derrière la tuyère
        if (frame % 2 == 0) {
            display.drawLine(rx - 1, ry + 1, rx - 5, ry + 5, SSD1306_WHITE);
        } else {
            display.drawLine(rx - 1, ry + 1, rx - 8, ry + 8, SSD1306_WHITE);
        }

        display.display();
        delay(25); // ~40 FPS, dure environ 1.25 secondes
    }

    delay(800); // Reste figé un court instant pour apprécier le titre
    return true;
}

void display_update(void) {
    if (!display_present) {
        return; // Évite les appels I2C si l'écran n'est pas branché physiquement
    }

    // 1. Gestion des états de vol critiques (Coupure de l'écran pour économie & sécurité)
    if (currentState == ASCEND || currentState == WINDOW || currentState == DESCEND) {
        if (!screen_turned_off) {
            display.clearDisplay();
            display.display();
            display.ssd1306_command(SSD1306_DISPLAYOFF); // Éteint physiquement l'écran
            screen_turned_off = true;
            log_entry("[OLED] Ecran eteint pour la phase de vol active.");
        }
        return; // Ne fait aucune écriture I2C pendant le vol
    }

    // Rallume l'écran si on vient de finir le vol
    if (screen_turned_off && currentState == TOUCHDOWN) {
        display.ssd1306_command(SSD1306_DISPLAYON);
        screen_turned_off = false;
        log_entry("[OLED] Ecran rallume apres atterrissage.");
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
    display.print("BAT: ");
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
        display.print("** RAPPORT VOL **");

        float max_alt = lps22hb_get_max_altitude();
        float max_vel = telemetry_get_max_velocity();
        float max_acc = telemetry_get_max_acceleration();

        display.setCursor(0, 26);
        display.print("Alt Max   : "); display.print(max_alt, 1); display.print(" m");

        display.setCursor(0, 35);
        display.print("Vit Max   : "); display.print(max_vel, 1); display.print(" m/s");

        display.setCursor(0, 44);
        display.print("Accel Max : "); display.print(max_acc, 2); display.print(" g");

        display.setCursor(0, 53);
        if (telemetry_is_apogee_recorded()) {
            display.print("Apogee    : T+"); 
            display.print((float)telemetry_get_apogee_time_ms() / 1000.0f, 1);
            display.print(" s");
        } else {
            display.print("Apogee    : Non detectee");
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
            display.print("ETAT : "); 
            display.print(state_to_string(currentState));

            display.setCursor(0, 26);
            display.print("RBF  : "); 
            display.print(rbf == 0 ? "RETIRE" : "INSERE");

            display.setCursor(0, 36);
            display.print("Jack : "); 
            display.print(jack == 0 ? "EJECTE" : "CONNECTE");

            display.setCursor(0, 46);
            display.print("Opto3: "); 
            display.print(gpio_get(PIN_OCTO_N3) ? "HIGH" : "LOW");

            display.setCursor(0, 56);
            display.print("Opto4: "); 
            display.print(gpio_get(PIN_OCTO_N4) ? "HIGH" : "LOW");
        } else {
            // Page 2 : Capteurs & Kalman
            float press = lps22hb_read_pressure();
            float alt = lps22hb_get_kalman_altitude();
            float vel = lps22hb_get_kalman_velocity();
            
            float ax = 0.0f, ay = 0.0f, az = 0.0f;
            lsm6dsl_read_accel(&ax, &ay, &az);
            float acc_mag = sqrtf(ax*ax + ay*ay + az*az);

            display.setCursor(0, 14);
            display.print("Pres : "); display.print(press, 1); display.print(" hPa");

            display.setCursor(0, 26);
            display.print("Alt  : "); display.print(alt, 1); display.print(" m");

            display.setCursor(0, 36);
            display.print("Vit  : "); display.print(vel, 1); display.print(" m/s");

            display.setCursor(0, 46);
            display.print("Acc  : "); display.print(acc_mag, 2); display.print(" g");

            display.setCursor(0, 56);
            display.print("Temp : "); display.print(temperature_read_mcu(), 1); display.print(" C");
        }
    }

    display.display();
}
