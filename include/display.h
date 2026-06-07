// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : display.h
// # Author  : Paul Miailhe
// # Date    : 2026-06-07
// # Object  : Gestion de l'écran OLED SSD1306 (I2C0 sur GP8/GP9)
// -------------------------------------------------------------

#ifndef DISPLAY_HEADER_FILE
#define DISPLAY_HEADER_FILE

#include "pico/stdlib.h"

/**
 * @brief Initialise l'écran OLED sur le bus I2C0, vérifie sa présence, 
 *        et affiche l'animation de démarrage du missile "MASTODONTE".
 * @return true si l'écran est détecté et initialisé, false sinon.
 */
bool display_init(void);

/**
 * @brief Met à jour l'affichage de l'écran selon l'état actuel du vol (FSM).
 *        Gère l'alternance des pages de diagnostic au sol et l'extinction en vol.
 */
void display_update(void);

#endif // DISPLAY_HEADER_FILE
