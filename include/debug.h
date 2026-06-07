// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : debug.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Activation et fonctions d'impression de debug sur port série
// -------------------------------------------------------------

#ifndef DEBUG_HEADER_FILE
#define DEBUG_HEADER_FILE

#define DEBUG_ENABLED true

#define DEBUG_TIMEOUT_MS 500

#include <Arduino.h>
#include <stdarg.h>

#if DEBUG_ENABLED

  /**
   * @brief Initialise le port série pour la transmission de debug.
   * @param baud Vitesse de transmission en bauds (par défaut 115200).
   * @return 1 si le port série est prêt avant le timeout, 0 sinon.
   */
  int debug_begin(unsigned long baud = 115200); 

  /**
   * @brief Affiche un message sur le port série (sans retour à la ligne).
   */
  void debug_print(const String& msg);

  /**
   * @brief Affiche un message sur le port série suivi d'un retour à la ligne.
   */
  void debug_println(const String& msg);

  /**
   * @brief Affiche un message formaté (type printf) sur le port série.
   * @param fmt Chaîne de format.
   * @param ... Liste d'arguments variables.
   */
  void debug_printf(const char* fmt, ...);

#else

  int debug_begin(unsigned long baud = 0);
  void debug_print(const String&);
  void debug_println(const String&);
  void debug_printf(const char*, ...);

#endif

#endif // DEBUG_HEADER_FILE
