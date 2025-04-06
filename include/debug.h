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

#define DEBUG_TIMEOUT_MS 4000

#include <Arduino.h>
#include <stdarg.h>

#if DEBUG_ENABLED
  int debugBegin(unsigned long baud = 115200); 
  void debugPrint(const String& msg);
  void debugPrintln(const String& msg);
  // Fonction variadique (comme printf) : accepte un format + arguments variables
  // Ex: debugPrintf("valeur = %d\n", 42);
  // Nécessite <stdarg.h> pour gérer les arguments (...).
  void debugPrintf(const char* fmt, ...);

#else
  int debugBegin(unsigned long baud = 0);
  void debugPrint(const String&);
  void debugPrintln(const String&);
  void debugPrintf(const char*, ...);

#endif

#endif // DEBUG_HEADER_FILE
