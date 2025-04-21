// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : log.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-21
// # Object  : Log interface for flash-based LittleFS (C++ only)
// -------------------------------------------------------------

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <LittleFS.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"


// Initialise le système de fichiers LittleFS en flash
bool log_init(void);

// Ajoute une ligne de texte dans le fichier de log
bool log_append(const char* message);

// Supprime le fichier de log existant
void log_clear(void);

#endif // LOG_H