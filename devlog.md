# Journal de Développement (Devlog) - FlightSoftware-Mastodonte

Ce journal sert à suivre la feuille de route du projet, les choix technologiques et l'état d'avancement des développements sur la branche `dev` pour l'avionique de la fusée Mastodonte (cible RP2040-YD).

---

## 📅 Historique des Modifications

### 23 Mai 2026 : Sécurisation des Interruptions (ISR)
*   **Auteur :** Paul Miailhe & Antigravity
*   **Problème résolu :** Appels non réentrants de `debug_printf` et `log_entryf` dans les callbacks d'alarmes et de GPIO. Risque de blocage (deadlock) de la carte en vol.
*   **Solution appliquée :**
    *   Les fonctions ISR (`seq_gpio_callback`, `seq_is_window_open_callback`, etc.) ne font plus que lever des drapeaux volatiles (les *triggers*).
    *   L'affichage de debug et l'écriture de logs flash correspondants sont déportés dans les gestionnaires d'état (`seq_preLaunch`, `seq_pyroRdy`, etc.) du fil principal (thread context).
*   **Statut :** Validé, compile avec succès (PlatformIO `SUCCESS`).

---

## 🚀 Feuille de Route & Fonctionnalités à Implémenter

### 1. Nettoyage Complet de la Surcouche Arduino (C/C++ Pur Pico SDK)
Le but est d'éliminer les bibliothèques Arduino pour réduire l'empreinte et avoir un contrôle matériel direct et prédictible.

*   [ ] **Buzzer (Remplacement de `tone` / `noTone`) :**
    *   *Méthode :* Utiliser le périphérique **PWM matériel** du RP2040. Configurer une fréquence de base (ex: 2kHz) et un rapport cyclique à 50% pour générer le son.
*   [ ] **LED RGB (Remplacement d' `Adafruit_NeoPixel`) :**
    *   *Méthode :* Implémenter le programme PIO standard `ws2812.pio` pour envoyer les couleurs (GRB) en arrière-plan sans bloquer le processeur.
*   [ ] **Debug (Remplacement de `Serial`) :**
    *   *Méthode :* Utiliser `stdio_init_all()` du Pico SDK et remplacer les écritures par des `printf(...)` C standards.

---

### 2. Intégration des Capteurs I2C (LSM6DSL & LPS22HB)
Utilisation de la carte de capteurs *BerryRocket* connectée sur le bus **I2C1** (SDA: **GP6**, SCL: **GP7**).

*   [ ] **Configuration de l'I2C1 :**
    *   Activer le périphérique `i2c1` à 400 kHz (Fast Mode).
*   [ ] **Driver LSM6DSL (IMU 6 axes) :**
    *   *Adresse I2C :* `0x6A` (broche SA0 à la masse).
    *   *Initialisation :* Vérifier `WHO_AM_I` (doit renvoyer `0x6A`). Configurer l'échelle de l'accéléromètre à $\pm 16g$ (registre `CTRL1_XL`) pour encaisser la poussée du décollage sans saturer.
*   [ ] **Driver LPS22HB (Baromètre) :**
    *   *Adresse I2C :* `0x5C` (broche SA0 à la masse).
    *   *Initialisation :* Vérifier `WHO_AM_I` (doit renvoyer `0xB1`). Activer le mode continu 50 Hz.
*   [ ] **Algorithme d'Apogée :**
    *   Acquérir l'altitude à 50 Hz, appliquer un filtre de moyenne mobile sur 10 échantillons.
    *   Détecter la transition de vitesse verticale positive à négative pendant la fenêtre `WINDOW` pour déclencher `DEPLOY_ALGO`.

---

### 3. Récupération des Logs via USB Mass Storage (MSC)
*Objectif :* Permettre de brancher la carte en USB sur un PC et d'y voir un disque dur virtuel (comme une clé USB) contenant le fichier `log.txt`, sans avoir à utiliser un logiciel de terminal série.

#### Option A : Mass Storage Réel (Accès direct à la Flash)
*   **Principe :** Utiliser la pile USB **TinyUSB** intégrée au RP2040. Exposer un système de fichier standard FAT12/FAT16 stocké en Flash (ou formater une partition pour cela).
*   **Fonctionnement :** La pile TinyUSB intercepte les requêtes de lecture/écriture de blocs (secteurs de 512 octets) du PC via les fonctions :
    *   `tud_msc_read10_cb(...)` : lit les données depuis la flash et les renvoie au PC.
    *   `tud_msc_write10_cb(...)` : écrit les données du PC sur la flash.

#### Option B : Mass Storage Simulé (Lecture seule virtuelle)
*   **Principe :** Plutôt que de stocker un vrai système FAT en mémoire flash, la carte RP2040 génère à la volée les tables FAT et les fichiers à la lecture de secteurs fictifs demandés par le PC.
*   **Avantages :** 
    *   Très léger en mémoire.
    *   Pas de risque de corruption de la flash par l'explorateur Windows.
    *   Le fichier `LOG.TXT` apparaît virtuellement au PC et affiche les données lues en direct depuis le système de fichier interne (LittleFS).

---

## 🛠️ Notes de Câblage (BerryRocket Sensor)

| Broche Capteur | Fonction | Broche RP2040 | Statut Logiciel |
| :--- | :--- | :--- | :--- |
| **SDA** | Bus Données I2C | **GP6** (J1 Pin 9) | I2C1 |
| **SCL** | Bus Horloge I2C | **GP7** (J1 Pin 10) | I2C1 |
| **INT1-IMU** | Interrupt IMU 1 | **GP3** (J1 Pin 5) | Entrée (Data Ready) |
| **INT-BARO** | Interrupt Baro | **GP4** (J1 Pin 6) | Optionnel |
| **INT2-IMU** | Interrupt IMU 2 | **GP27** (J2 Pin 12) | **NON UTILISÉ** (Conflit avec RBF) |

> [!WARNING]
> **Rappel de Conflit Matériel (GP27) :**
> La broche **GP27** est connectée physiquement à `INT2-IMU` sur la BerryRocket, mais elle sert également au commutateur RBF (câble arraché) de la carte principale. **Ne jamais configurer la broche `INT2` sur l'IMU** pour éviter que l'IMU et le commutateur n'entrent en conflit de tension sur la broche GP27.
