// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : log.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-21
// # Object  : Log interface for flash-based LittleFS (C++ only)
// -------------------------------------------------------------

#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <LittleFS.h>

#include "system.h"
#include "debug.h" // Pour debug_printf()

// Initialise le système de fichiers LittleFS en flash
// Retourne true si montage réussi, sinon false
bool log_init(void);

// Ajoute un message brut dans le fichier de log (sans timestamp)
bool log_append(const char* message);

// Ajoute une entrée horodatée formatée automatiquement dans le log
bool log_entry(const char* event);

// Affiche le contenu du log sur la sortie série (debug_printf)
void log_dump(void);

// Supprime le fichier de log existant de la flash
void log_clear(void);

// Vérifie si la mémoire est presque pleine (moins de 4 blocs libres)
// Retourne true si proche de saturation
bool log_near_full(void);

#endif // LOG_H