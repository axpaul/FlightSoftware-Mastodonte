// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : debug.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Debug printf activation
// -------------------------------------------------------------

#ifndef DEBUG_HEADER_FILE
#define DEBUG_HEADER_FILE

#define DEBUG_ENABLED true

#define DEBUG_TIMEOUT_MS 10000

#include <Arduino.h>
#include <stdarg.h>

#if DEBUG_ENABLED
  int debug_begin(unsigned long baud = 115200); 
  void debug_print(const String& msg);
  void debug_println(const String& msg);
  void debug_printf(const char* fmt, ...); // Fonction variadique (comme printf) : accepte un format + arguments variables. Ex: debugPrintf("valeur = %d\n", 42);

#else
  int debug_begin(unsigned long baud = 0);
  void debug_print(const String&);
  void debug_println(const String&);
  void debug_printf(const char*, ...);

#endif

#endif // DEBUG_HEADER_FILE
