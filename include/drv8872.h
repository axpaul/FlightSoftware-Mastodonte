// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : drv8872.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Pilote pour pont en H DRV8872 (contrôle des moteurs CC)
// -------------------------------------------------------------

#ifndef DRV8872_H
#define DRV8872_H

#include <stdlib.h>              // Pour malloc() et free()
#include "pico/stdlib.h"
#include "hardware/pwm.h"        // Accès au PWM matériel du Pico SDK

#include "debug.h"
#include "platform.h"            // Définitions des broches INx_Mx, NFAUT_Mx
#include "log.h"

/**
 * @brief Structure décrivant les pins et la callback d'un pilote de moteur DRV8872.
 */
typedef struct {
    uint in1_pin;                   // Broche d'entrée IN1
    uint in2_pin;                   // Broche d'entrée IN2
    uint fault_pin;                 // Broche nFAULT (active LOW en cas de défaut)
    void (*fault_callback)(void);   // Pointeur vers la fonction à exécuter en cas de défaut
} drv8872_t;

/**
 * @brief Structure de regroupement de plusieurs moteurs pour activation simultanée.
 */
typedef struct {
    drv8872_t* motors[3];           // Tableau de pointeurs vers 3 moteurs maximum
    uint8_t motor_count;            // Nombre de moteurs actifs dans le groupe (1 à 3)
    bool direction;                 // Sens de rotation : true = avant, false = arrière
} drv8872_group_t;

// --- Moteurs et groupes globaux ---
extern drv8872_t motor1;
extern drv8872_t motor2;
extern drv8872_t motor3;
extern drv8872_t* motors[3];
extern drv8872_group_t group_all_motors;

extern bool motor1_ok;
extern bool motor2_ok;
extern bool motor3_ok;

/**
 * @brief Initialise les broches d'un moteur et assure son arrêt complet de départ.
 */
void drv8872_init(drv8872_t* drv);

/**
 * @brief Accède à une instance de moteur via son index (0, 1 ou 2).
 */
drv8872_t* drv8872_get(uint8_t index);

// --- Commandes basiques de rotation ---

/**
 * @brief Fait tourner le moteur en marche avant.
 */
void drv8872_forward(drv8872_t* drv);

/**
 * @brief Fait tourner le moteur en marche arrière.
 */
void drv8872_reverse(drv8872_t* drv);

/**
 * @brief Freine activement le moteur (IN1 et IN2 à HIGH).
 */
void drv8872_brake(drv8872_t* drv);

/**
 * @brief Arrête le moteur en roue libre (IN1 et IN2 à LOW).
 */
void drv8872_stop(drv8872_t* drv);

// --- Commandes de vitesse variable (PWM) ---

/**
 * @brief Fait tourner le moteur en marche avant avec un niveau de vitesse PWM défini.
 */
void drv8872_forward_pwm(drv8872_t* drv, uint slice_num, uint level);

/**
 * @brief Fait tourner le moteur en marche arrière avec un niveau de vitesse PWM défini.
 */
void drv8872_reverse_pwm(drv8872_t* drv, uint slice_num, uint level);

// --- Diagnostics et sécurité ---

/**
 * @brief Vérifie si le moteur est actuellement dans un état de défaut (surchauffe, court-circuit, etc.).
 * @return true si un défaut est actif, false sinon.
 */
bool drv8872_is_fault(drv8872_t* drv);

/**
 * @brief Configure l'interruption matérielle pour surveiller la broche nFAULT du moteur.
 */
void drv8872_setup_fault_interrupt(drv8872_t* drv);

/**
 * @brief Gère l'aiguillage du défaut moteur selon le GPIO qui a déclenché l'interruption.
 */
void drv8872_handle_fault(uint gpio);

// --- Gestion des groupes de moteurs temporisés ---

/**
 * @brief Active le groupe de moteurs pour une durée précise (en microsecondes) puis l'arrête automatiquement.
 */
void drv8872_group_activate_for_us(drv8872_group_t* group, uint64_t duration_us);

// --- Fonctions de test et de diagnostic ---

/**
 * @brief Initialise les trois moteurs pour le diagnostic.
 */
void motor_diag_init(void);

/**
 * @brief Lance une séquence complète de test pour valider le fonctionnement physique des moteurs.
 */
void motor_diag_test_sequence(void);

/**
 * @brief Lit et affiche sur la console de debug l'état de défaut des 3 moteurs.
 */
void motor_diag_log_faults(void);

/**
 * @brief Teste activement la présence d'un court-circuit sur le moteur en lui envoyant une impulsion de 2ms.
 * @return true si un défaut (court-circuit/surcharge) est détecté, false sinon.
 */
bool drv8872_test_short_circuit(drv8872_t* drv);

/**
 * @brief Exécute un autotest actif sur tous les moteurs déclarés.
 * @return true si TOUS les moteurs ont passé l'autotest avec succès, false si un défaut est détecté.
 */
bool motor_diag_run_self_test(void);

#endif // DRV8872_H
