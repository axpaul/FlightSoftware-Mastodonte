# 📓 Journal de Développement (Devlog) - FlightSoftware-Mastodonte

Ce journal sert à suivre la feuille de route du projet, les choix technologiques et l'état d'avancement des développements sur la branche `dev` pour l'avionique de la fusée Mastodonte (cible RP2040-YD).

---

## 🛠️ Comment utiliser ce Journal de Dev ?
- Utilisez les cases à cocher `[ ]` pour suivre l'avancement. Cochez-les en changeant `[ ]` par `[x]` dès qu'une tâche est validée.
- Les sections détaillent le fonctionnement théorique et fournissent des structures ou pistes d'implémentation pour coder proprement sans tâtonner.

---

## 📅 Historique & État d'Avancement

| Fonctionnalité | Cible Matérielle | Fichier(s) Source | Statut |
| :--- | :--- | :--- | :--- |
| **Sécurisation des ISR** | GPIO / Alarms RP2040 | `src/sequencer.cpp` | ✅ Terminée (23 Mai 2026) |
| **Buzzer (Pico SDK)** | GP2 (Passive Buzzer) | `src/buzzer.cpp` | ⏳ À Faire |
| **LED RGB (PIO ws2812)** | GP23 (NeoPixel YD-RP2040) | `src/system.cpp` | ⏳ À Faire |
| **Drivers Capteurs I2C** | LSM6DSL + LPS22HB | `src/sensors.cpp` (à créer) | ⏳ À Faire |
| **Algorithme Apogée & Stats** | I2C / Altimétrie & IMU | `src/sequencer.cpp` | ⏳ À Faire |
| **USB Mass Storage (MSC)** | Connecteur USB RP2040 | `src/usb_msc.cpp` (à créer) | ⏳ À Faire |

---

## 🚀 Plan de Développement Détaillé

### 1. Nettoyage de la surcouche Arduino (Transition Pico SDK)
Pour maximiser la prédictibilité temporelle et le contrôle matériel, nous remplaçons les appels Arduino par l'API C native du Pico SDK.

- [ ] **Buzzer matériel (PWM) :**
  - Remplacer `tone()` et `noTone()` par le périphérique **PWM** du RP2040.
  - *Algorithme :*
    1. Récupérer le "slice" PWM associé au GP2 (`pwm_gpio_to_slice(2)`).
    2. Configurer le diviseur d'horloge et la période pour obtenir la fréquence voulue (ex: 2 kHz).
    3. Mettre le niveau à 50% du cycle de travail (duty cycle) pour faire chanter le buzzer, et à 0% pour l'éteindre.
- [ ] **LED RGB onboard (PIO) :**
  - Retirer `Adafruit_NeoPixel` pour éviter les blocages de longueurs d'onde.
  - *Algorithme :* Charger le programme assembleur PIO `ws2812.pio` (standard Pico SDK) dans l'une des machines d'état PIO pour piloter la LED RGB sur le GP23.

---

### 2. Intégration des Capteurs & Algorithmes Simplifiés (Monocœur)
Pour garantir la robustesse du système sans complexité inutile, nous lisons les capteurs sur le **Core 0** et utilisons des algorithmes légers en mémoire RAM (sans écriture flash haute fréquence).

- [ ] **Initialisation I2C1 (Pico SDK) :**
  - Configurer `i2c1` à 400 kHz (`i2c_init(i2c1, 400 * 1000)`).
  - Configurer GP6 et GP7 en fonction I2C (`gpio_set_function(...)`).
- [ ] **Driver LSM6DSL (IMU) & Paramètres Max :**
  - Valider `WHO_AM_I` (`0x6A`). Configurer l'échelle à **$\pm 16g$** (`CTRL1_XL`).
  - Dans la boucle principale, mesurer à chaque cycle l'accélération totale $a_{\text{tot}} = \sqrt{a_x^2 + a_y^2 + a_z^2}$ et la vitesse angulaire totale $\omega_{\text{tot}} = \sqrt{g_x^2 + g_y^2 + g_z^2}$.
  - Enregistrer en RAM dans des variables globales les maximums historiques : `max_accel_g` et `max_gyro_dps`.
- [ ] **Driver LPS22HB (Baromètre) & Détection d'Apogée :**
  - Valider `WHO_AM_I` (`0xB1`). Activer le mode continu 50 Hz.
  - Calculer l'altitude relative $z$ à partir de la pression.
  - Appliquer un filtre passe-bas du premier ordre très léger :
    $$z_{\text{filtree}} = \alpha \times z_{\text{brute}} + (1 - \alpha) \times z_{\text{filtree\_precedente}}$$
    *(avec $\alpha \approx 0.15$ pour lisser le bruit barométrique).*
  - Suivre l'altitude maximale atteinte pendant le vol (`max_altitude`).
  - Détecter l'apogée si l'altitude actuelle descend en dessous de `max_altitude - 2.0` (mètres) pendant la fenêtre d'ouverture.
- [ ] **Stratégie d'Enregistrement Allégée :**
  - Ne **jamais** enregistrer les mesures brutes en continu pour éviter de bloquer le CPU lors des accès flash.
  - Écrire dans le fichier `LOG.TXT` **uniquement** lors des transitions d'état clés (Décollage détecté, Apogée détectée, Ouverture parachute, Atterrissage).
  - À l'atterrissage (`TOUCHDOWN`), écrire un rapport final unique récapitulant les statistiques de vol stockées en RAM : altitude max, accélération max, vitesse angulaire max, durée de la montée.

---

### 3. USB Mass Storage Simulé (Lecture Seule de `LOG.TXT`)
Pour récupérer les logs de vol sans logiciel de terminal série, nous allons simuler un disque flash USB (FAT12) à la volée. 
Plutôt que d'écrire un vrai système FAT complexe sur la flash, le RP2040 intercepte les requêtes USB du PC et **génère virtuellement les secteurs FAT12** pour faire apparaître un unique fichier `LOG.TXT` lié au fichier réel stocké dans LittleFS.

#### Architecture de la FAT12 Virtuelle (Secteurs de 512 octets) :
*   **Secteur 0 :** Boot Sector (BPB) contenant la géométrie du disque (ex: disquette 1.44 Mo standard).
*   **Secteurs 1 à 9 :** Table FAT 1 (pointeurs de blocs).
*   **Secteurs 10 à 18 :** Table FAT 2 (copie de sauvegarde standard).
*   **Secteurs 19 à 32 :** Répertoire racine (Root Directory) contenant l'entrée du fichier `LOG     TXT`.
*   **Secteurs 33+ :** Zone de Données (Clusters). Le cluster 2 correspond au secteur 33.

#### Implémentation du Générateur de Secteurs à la volée :

- [ ] **Boot Sector (Secteur 0) :**
  Renvoyer une structure `FAT12_BootSector` fixe et normalisée.
- [ ] **Table FAT (Secteurs 1 à 18) :**
  Puisque le fichier `LOG.TXT` est composé de $C$ secteurs (où $C = \text{arrondiSuperieur}(\text{taille\_fichier} / 512)$) :
  - L'entrée FAT $2 \le i < 2+C-1$ pointe vers le cluster suivant $i+1$.
  - L'entrée $i = 2+C-1$ contient le marqueur de fin de fichier `0xFFF`.
  - Toutes les autres entrées contiennent `0x000` (libre).
  - Le code calcule la valeur de la FAT au vol pour le secteur demandé (12 bits par entrée, soit 3 octets pour 2 entrées) sans stocker toute la table en RAM.
- [ ] **Root Directory (Secteur 19) :**
  Renvoyer une structure `FAT12_DirEntry` pour `LOG.TXT` à l'offset 0 du secteur 19, avec la taille réelle lue depuis LittleFS et le cluster de départ configuré à `2`. Les autres octets de la table racine (secteurs 20 à 32) restent à `0`.
- [ ] **Data Sectors (Secteurs 33+) :**
  Si le PC demande le secteur $S \ge 33$, cela correspond au cluster de données $Cl = S - 33 + 2$.
  - Si le cluster est dans la plage de `LOG.TXT` ($2 \le Cl < 2+C$), ouvrir le fichier réel de logs sous LittleFS, faire un `seek((Cl - 2) * 512)`, lire 512 octets et les renvoyer en USB.
  - Sinon, renvoyer des zéros.
- [ ] **Sécurisation en écriture :**
  Le callback TinyUSB `tud_msc_write10_cb` doit simplement ignorer les écritures et renvoyer la taille demandée (pour simuler un succès mais bloquer toute altération par l'OS).
- [ ] **Sécurité d'Accès Concurrant :**
  Désactiver l'écriture de logs flash de l'algorithme de vol dès qu'une connexion USB active est détectée (via la broche VBUS ou un état TinyUSB) pour éviter que le système n'écrive sur la flash en LittleFS en même temps que le PC lit.

---

## 🛠️ Notes de Câblage (BerryRocket Sensor)

| Broche Capteur | Fonction | Broche RP2040 | Variable Logicielle (`platform.h`) |
| :--- | :--- | :--- | :--- |
| **SDA** | Bus Données I2C | **GP6** (J1 Pin 9) | `PIN_I2C1_SDA` |
| **SCL** | Bus Horloge I2C | **GP7** (J1 Pin 10) | `PIN_I2C1_SCL` |
| **INT1-IMU** | Interrupt IMU 1 | **GP3** (J1 Pin 5) | `PIN_OCTO_N3` |
| **INT-BARO** | Interrupt Baro | **GP4** (J1 Pin 6) | `PIN_OCTO_N4` |
| **INT2-IMU** | Interrupt IMU 2 | **GP27** (J2 Pin 12) | **NON UTILISÉ** (Conflit RBF sur `PIN_SMITCH_N2`) |

> [!WARNING]
> **Rappel de Conflit Matériel (GP27) :**
> La broche **GP27** (`PIN_SMITCH_N2`) est connectée physiquement à `INT2-IMU` sur la BerryRocket, mais elle sert également au commutateur RBF (câble arraché) sur la carte principale de vol. **Ne jamais configurer/activer la broche `INT2` sur l'IMU** sous peine de court-circuit ou de mauvaise lecture de l'état RBF.
