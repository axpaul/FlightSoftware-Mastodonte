// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : log.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-21
// # Object  : Log interface for flash-based LittleFS (C++ only)
// -------------------------------------------------------------

#ifndef LOG_H
#define LOG_H

#define LOG_ENABLED 1  // Utiliser 0 pour désactiver complètement le log

#include <Arduino.h>
#include <LittleFS.h>
#include <cstdarg>  // pour va_list, va_start, vsnprintf
#include <queue>
#include <mutex>
#include <string>
#include <deque>

#include "system.h"
#include "debug.h" // Pour debug_printf()

#if LOG_ENABLED

    bool log_init(void);
    bool log_append(const char* message);
    bool log_entry(const char* event);
    bool log_entryf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
    void log_dump(void);
    void log_clear(void);
    bool log_near_full(void);
    bool log_has_space(void);
    void log_flush(void);
    
#else

    // Fonctions neutres quand le logging est désactivé
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
