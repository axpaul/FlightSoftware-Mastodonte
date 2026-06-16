# Logiciel de Vol Mastodonte

**FlightSoftware-Mastodonte** est le logiciel de vol embarqué s'exécutant sur le contrôleur **YD-RP2040** de la fusée expérimentale **Mastodonte**.  
Il fait office de **séquenceur de vol**, gérant les événements critiques en temps réel et les interfaces matérielles durant la mission.

> *Copyright (c) 2025/2026 - Paul Miailhe.*  
> *This firmware source code is licensed under GNU General Public License v3. [→ GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0)*

[![BerryRocket](https://img.shields.io/badge/Berry-Rocket-red?style=flat-square&logo=rocket&logoColor=white)](https://berryrocket.com)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-red?style=flat-square&logo=gnu&logoColor=white)](https://www.gnu.org/licenses/gpl-3.0)

---

<p align="center">
  <img src="docs/Mastodonte-N6.png" alt="Carte Mastodonte" width="800"/>
</p>

---

## Fonctionnalités

- **Cœur natif Pico SDK** : Pilote I2C bas niveau personnalisé (accès direct aux registres) pour les capteurs sur le bus `i2c1` (GP6/GP7) fonctionnant à 400 kHz.
- **Filtrage de Kalman 1D** : Estimateur fonctionnant à 25 Hz synchronisé sur l'interruption matérielle `GP5` (DRDY) pour suivre l'altitude et la vitesse verticale.
- **Détection d'apogée** : Détection multi-capteurs redondante combinant deux optocoupleurs et le déclencheur d'apogée du filtre de Kalman ($z > 15\text{ m}$ et $v_z \le -1.0\text{ m/s}$ sur 5 échantillons consécutifs).
- **Autotest de sécurité et alarme** : Autotest automatique de l'état des capteurs au démarrage ; si le baromètre est absent, le système émet 3 bips (au lieu de 2) et bascule en toute sécurité en **Mode de vol fenêtré uniquement** (secours temporel).
- **Surveillance de batterie non bloquante** : Mesure continue de la tension de la batterie via un timer matériel (fréquence de 1 Hz) avec alertes LED intelligentes lorsque $V_{cc} \le 6.0\text{V}$.
- **Contrôleur de pont en H pour la récupération** : Contrôle autonome et sécurisé des moteurs de récupération avec désactivation temporaire de la détection de défaut au décollage (évite les déclenchements intempestifs dus aux pics de courant).
- **Liaison Série USB & LittleFS** : Vidage des logs de vol via liaison série USB, et bouton physique GP24 pour effacer ou lire les logs stockés sur le système de fichiers LittleFS de la flash.
- **Support CMSIS-DAP** : Flashage et débogage matériel via SWD.

---

## Architecture Logicielle

Le logiciel de vol est structuré de manière modulaire en C++ afin de garantir la prévisibilité, la sécurité d'exécution et la séparation des responsabilités.

```mermaid
graph TD
    Main[main.cpp <br/> Orchestrator] -->|Initialise et cadence| Sys[system.cpp <br/> Abstraction Matérielle et Santé]
    Main -->|Cadence| Seq[sequencer.cpp <br/> Séquenceur de Vol FSM]
    Seq -->|Enregistre les événements| Log[log.cpp <br/> Journalisation LittleFS Flash]
    Seq -->|Déclenche| Buzzer[buzzer.cpp <br/> Retours audio PWM]
    Seq -->|Contrôle| Motors[drv8872.cpp <br/> Pont en H de Récupération]
    Seq -->|Acquires via GP5 DRDY| Baro[lps22hb.cpp <br/> Baromètre bas niveau]
    Baro -->|Estime altitude/vitesse| Kalman[Filtre de Kalman 1D]
    Seq -->|Lit l'accélération| IMU[lsm6dsl.cpp <br/> IMU bas niveau]
```

### Description des Modules

- **`main.cpp` (Orchestrateur)** : Gère la phase de démarrage, effectue l'autotest des moteurs et des capteurs, configure les interruptions GPIO et exécute la boucle principale.
- **`sequencer.cpp` (Logique de vol)** : Pilote la machine d'état (FSM) de vol (de `PRE_FLIGHT` à `TOUCHDOWN`), écoute les broches d'interruption (Jack, RBF) et active les moteurs de déploiement.
- **`lps22hb.cpp` & `lsm6dsl.cpp` (Capteurs)** : Implémentations bas niveau natives Pico SDK pour le baromètre et l'IMU.
- **`system.cpp` (Abstraction matérielle)** : Initialise les GPIOs, la LED RGB statut (WS2812B) et la mesure de tension batterie (ADC).
- **`buzzer.cpp` (Retours audio)** : Génère des motifs sonores non bloquants à l'aide d'un slice PWM matériel pour indiquer l'état du système.
- **`log.cpp` (Journalisation)** : Enregistre de façon non bloquante les événements de vol dans la flash interne via LittleFS.
- **`drv8872.cpp` (Moteurs)** : Contrôle les ponts en H pour les moteurs d'éjection des parachutes.

---

## Configuration Matérielle

| Paramètre           | Valeur                    |
|---------------------|--------------------------|
| Carte               | `YD-RP2040`              |
| Microcontrôleur     | RP2040                   |
| Framework           | Arduino (earlephilhower) |
| Protocole de debug  | CMSIS-DAP (SWD)          |
| Découpage Flash     | 1 Mo firmware / 15 Mo FS |
| Fréquence d'horloge | 133 MHz                  |
| Pile USB            | TinyUSB                  |
| Chaîne de compil.   | `toolchain-rp2040-earlephilhower` |
| Système de build    | PlatformIO               |
| Pin Mesure Batterie | GP28 (ADC2)              |
| Pin Interrup. Baro  | GP5                      |

---

## Bibliothèques Externes

Déclarées dans `platformio.ini` :

| Bibliothèque                 | Version   | Rôle                             |
|------------------------------|-----------|----------------------------------|
| Adafruit NeoPixel            | `1.12.5`  | Contrôle de la LED WS2812B       |

---

## Synoptique

<p align="center">
  <img src="docs/Mastodonte_synoptique.png" alt="Synoptique Mastodonte" width="750"/>
</p>

---

## 🚀 Guide de Démarrage Rapide (Quickstart)

### 1. Prérequis Logiciels
*   Installer [VS Code](https://code.visualstudio.com/).
*   Installer l'extension **PlatformIO IDE** depuis le gestionnaire d'extensions de VS Code.

### 2. Compilation du Projet
1.  Ouvrir VS Code, puis ouvrir le dossier du projet `FlightSoftware-Mastodonte`.
2.  Laisser PlatformIO configurer l'environnement (cela peut prendre quelques minutes au premier démarrage).
3.  Cliquer sur l'icône de **coche (✓)** dans la barre d'état inférieure de VS Code (Build) ou exécuter la commande suivante dans le terminal :
    ```bash
    pio run
    ```

### 3. Téléversement / Flashage de la Carte YD-RP2040
Le téléversement se fait via USB :
1.  Brancher la carte YD-RP2040 à votre ordinateur avec un câble USB-C.
2.  Maintenir enfoncé le bouton **BOOT** (situé sur la carte), appuyer brièvement sur le bouton **RST** (Reset), puis relâcher le bouton BOOT.
3.  La carte apparaît sur l'ordinateur comme une clé USB nommée **`RPI-RP2`**.
4.  Cliquer sur la **flèche (→)** dans la barre d'état de PlatformIO (Upload) pour flasher. Vous pouvez aussi copier-coller manuellement le fichier binaire `.pio/build/pico/firmware.uf2` directement sur la clé USB `RPI-RP2`. La carte redémarrera automatiquement dès la fin du transfert.

### 4. Console de Débogage (Serial Monitor)
*   Ouvrir le moniteur série de PlatformIO (l'icône de prise électrique) pour visualiser la télémétrie et le statut du vol en direct à **`115200 bauds`**.
*   **Comportement au Boot** : Si `DEBUG_ENABLED` est activé, la carte attend jusqu'à 10 secondes l'ouverture de la console série USB avant de démarrer pour que vous ne manquiez pas les premiers logs d'initialisation.

---

## Tolérance aux Pannes & Diagnostics

### Signaux Sonores de Démarrage (Beep Codes)
Au démarrage, le buzzer fournit un diagnostic audio immédiat de l'état du système :
- **2 bips courts (1500 Hz)** : Initialisation du baromètre (`LPS22HB`) réussie.
- **3 bips plus longs (1200 Hz)** : Échec de l'initialisation du baromètre ou capteur physiquement absent. Le système bascule automatiquement en **Mode de Vol Fenêtré Uniquement** (sécurité temporelle).
- **2 bips courts (1500 Hz) après maintien de USR (GP24) pendant 5s** : Effacement réussi du fichier de logs sur la flash.

### Résilience en Vol face à une Déconnexion du Baromètre
Si le baromètre est déconnecté physiquement ou perd son alimentation pendant le vol :
1. **Sécurité CPU (Anti-figeage)** : Le contrôleur I2C du RP2040 détecte l'absence de réponse (NACK). Le pilote bas niveau non bloquant retourne instantanément un code d'erreur (`PICO_ERROR_GENERIC`) sans bloquer la boucle principale ni le séquenceur.
2. **Désactivation des Interruptions** : Le baromètre générant lui-même les interruptions sur `GP5` (DRDY), sa déconnexion arrête l'envoi de ces signaux. Le callback `lps22hb_drdy_callback` cesse donc d'être exécuté, stoppant toute tentative de transaction I2C vers le capteur.
3. **Timers de Secours du Séquenceur** :
   - **Apogée / Déploiement** : Si le filtre de Kalman ne peut plus détecter l'apogée, le timer de fin de fenêtre de vol (`WINDOW_DURATION_US`) prend le relais pour déclencher le déploiement de secours (`DEPLOY_TIMER`).
   - **Atterrissage (Touchdown)** : Si la stabilisation au sol ne peut plus être calculée par Kalman, un timer de descente théorique (`THEORETICAL_DESCENT_US`) force le passage à l'état final sécurisé `TOUCHDOWN`.

---

## États du Séquenceur de Vol

La logique de vol est pilotée par une machine d'état finie (`sequencer.cpp`). Chaque état configure une couleur de LED RGB et un motif sonore spécifique pour un retour d'information clair à l'opérateur.

| État            | Couleur (LED)    | Motif du Buzzer                      | Description |
|-----------------|------------------|--------------------------------------|-------------|
| `PRE_FLIGHT`    | 🟢 Vert          | 🔈 Double bip doux (pause de 3s)     | Fusée au sol, en attente de l'armement (RBF inséré, Jack connecté). |
| `PYRO_RDY`      | 🟡 Jaune         | 🔈 1 bip grave par seconde           | Armé et prêt pour le décollage (RBF retiré, Jack toujours connecté). |
| `ASCEND`        | 🔵 Bleu          | 🔈 Bips très rapides                 | Décollage détecté (retrait du Jack), fusée en phase de montée. |
| `WINDOW`        | 🔵 Cyan          | 🔈 Bips d'alerte rapides             | Fenêtre d'ouverture de la récupération ouverte. |
| `DEPLOY_ALGO`   | 🟠 Orange        | 🔈 Bips alternés moyens              | Déploiement activé par détection algorithmique (capteurs). |
| `DEPLOY_TIMER`  | 🟠 Orange        | 🔈 Bips alternés moyens              | Déploiement activé sur secours temporel. |
| `DESCEND`       | 🟣 Magenta       | 🔈 Bips lents et réguliers           | Descente sous parachute. |
| `TOUCHDOWN`     | 🟢 Vert (fixe)   | 🔈 Long bip toutes les 30 secondes   | Atterrissage détecté, sécurisation au sol. |
| `ERROR_SEQ`     | 🔴 Rouge         | 🔈 Bips aigus très rapides           | Défaut système ou transition d'état invalide. |

> [!TIP]
> **Alerte de Tension Batterie Basse :**
> Si la tension descend sous **6.0V** pendant la préparation (états `PRE_FLIGHT` ou `PYRO_RDY`), la LED clignote alternativement entre la **couleur de l'état actuel** et le **Rouge** toutes les secondes. Cette alerte n'est pas bloquante pour permettre un lancement forcé, mais signale une batterie faible.

---

## 🔘 Bouton Utilisateur USR (GP24) — Lecture et Effacement

Le bouton connecté sur **GPIO 24** est lu au démarrage :
- **Appui court** : Vide le contenu des fichiers de logs vers la liaison série USB à **115200 bauds**.
- **Appui long (5 secondes)** : Efface complètement le fichier de logs de la mémoire flash interne.

---

## 🛰️ Option : Carte Capteurs BR Mini Sensor

Ce logiciel prend en charge l'acquisition et le filtrage des données de vol (Kalman 1D) via la carte mezzanine **BR Mini Sensor** (équipée d'un accéléromètre/gyromètre LSM6DSL et d'un baromètre LPS22HB).

Pour plus d'informations sur le câblage, le brochage et les détails matériels de cette carte, veuillez vous référer à la documentation officielle :
👉 **[Wiki BR Mini Sensor](https://berryrocket.com/wiki/BR_Mini_Sensor)**
