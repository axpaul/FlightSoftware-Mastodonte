// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : debug.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Debug printf activation
// -------------------------------------------------------------

#include <Arduino.h>
#include "debug.h"

#if DEBUG_ENABLED

int debug_begin(unsigned long baud) {
    Serial.begin(baud);
    float start = millis();
  
    // Attente de connexion série jusqu'à timeout
    while (!Serial && (millis() - start < DEBUG_TIMEOUT_MS)) {
      delay(10);
    }
  
    if (Serial) {
      Serial.println("[DEBUG] Serial initialized.");
      return 1;  // OK
    }
  
    return 0;    // NOK
  }
  
void debug_print(const String& msg) {
  Serial.print(msg);
}

void debug_println(const String& msg) {
  Serial.println(msg);
}

void debug_printf(const char* fmt, ...) {
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.print(buffer);
}

#else

int debug_begin(unsigned long baud) {return 0;}
void debug_print(const String&) {}
void debug_println(const String&) {}
void debug_printf(const char*, ...) {}

#endif
