// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : log.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-21
// # Object  : Interface d'écriture des logs sur la flash (LittleFS)
// -------------------------------------------------------------

#ifndef LOG_H
#define LOG_H

#define LOG_ENABLED 1  // Utiliser 0 pour désactiver complètement l'écriture des logs

#include <Arduino.h>
#include <LittleFS.h>
#include <cstdarg>     // pour va_list, va_start, vsnprintf
#include <queue>
#include <mutex>
#include <string>
#include <deque>

#include "system.h"
#include "debug.h"     // Pour debug_printf()

#if LOG_ENABLED

    /**
     * @brief Initialise et monte le système de fichiers LittleFS pour les logs.
     * @return true en cas de succès, false sinon.
     */
    bool log_init(void);

    /**
     * @brief Ajoute directement une ligne de texte brute à la fin du fichier de log.
     */
    bool log_append(const char* message);

    /**
     * @brief Enregistre un événement textuel horodaté dans le buffer de logs.
     */
    bool log_entry(const char* event);

    /**
     * @brief Enregistre un événement horodaté formaté (façon printf) dans le buffer de logs.
     */
    bool log_entryf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

    /**
     * @brief Affiche l'intégralité du fichier de logs sur la liaison série (UART).
     */
    void log_dump(void);

    /**
     * @brief Efface définitivement le fichier de logs de la mémoire flash.
     */
    void log_clear(void);

    /**
     * @brief Vérifie si la mémoire flash allouée aux logs est bientôt pleine.
     */
    bool log_near_full(void);

    /**
     * @brief Vérifie s'il reste de l'espace disponible pour écrire des logs.
     */
    bool log_has_space(void);

    /**
     * @brief Écrit de manière non bloquante toutes les entrées en attente du buffer vers la flash.
     */
    void log_flush(void);
    
#else

    // Fonctions vides (stubs) optimisées pour économiser la mémoire quand les logs sont désactivés
    static inline bool log_init(void) { return true; }
    static inline bool log_append(const char* message) { (void)message; return true; }
    static inline bool log_entry(const char* event) { (void)event; return true; }
    static inline bool log_entryf(const char* fmt, ...) { (void)fmt; return true; }
    static inline void log_dump(void) {}
    static inline void log_clear(void) {}
    static inline bool log_near_full(void) { return false; }
    static inline bool log_has_space(void) { return true; }
    static inline void log_writer_task(void* arg) { (void)arg; }

#endif 

#endif // LOG_H
