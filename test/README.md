# Tests Unitaires du Logiciel de Vol Mastodonte (RP2040)

Ce dossier contient la suite de tests unitaires utilisée pour valider le comportement logique, mathématique et matériel du logiciel de vol de la fusée **Mastodonte** sur le microcontrôleur RP2040. Les tests s'appuient sur le framework **Unity** intégré à PlatformIO.

---

## 1. Objectifs des Tests sur Cible (On-Target Testing)

L'exécution des tests directement sur la carte physique RP2040 permet de garantir plusieurs aspects critiques du vol :

* **Validation des Calculs Physiques (Filtre de Kalman) :**
  Assure que l'implémentation du filtre de Kalman 1D et de la formule barométrique internationale `lps22hb_hpa_to_altitude()` s'exécutent avec la précision attendue sur l'architecture ARM Cortex-M0+ du RP2040 (sans erreurs d'arrondi ou d'optimisation du compilateur).
* **Validation de la Machine d'État (FSM) :**
  Vérifie les transitions de sécurité de la fusée (Armement via retrait RBF, Décollage via éjection du Jack, détection d'apogée et atterrissage) sous des stimuli simulés précis.
* **Vérification Matérielle des Périphériques (ADC) :**
  Valide que les fonctions de lecture de l'ADC matériel (tension de batterie et température interne du MCU) s'exécutent correctement, sont configurées sur les bons canaux et renvoient des valeurs cohérentes dans les limites physiques réelles.
* **Précision Temporelle :**
  Vérifie le découpage précis du temps système (`compute_timestamp()`) indispensable à l'horodatage des événements critiques en vol et dans les logs LittleFS.

---

## 2. Liste des Tests Implémentés (`test_main.cpp`)

La suite réalise **11 cas de tests** répartis en trois catégories :

### Séquenceur de vol (FSM)
* `test_fsm_initial_state` : Vérifie que le système démarre bien en état `PRE_FLIGHT`.
* `test_fsm_transition_rbf_remove` : Valide la transition vers `PYRO_RDY` lors du retrait du commutateur RBF.
* `test_fsm_transition_rbf_insert` : Valide le retour à `PRE_FLIGHT` en cas de réinsertion de sécurité du RBF.
* `test_fsm_transition_jack_remove` : Valide la transition vers `ASCEND` (décollage) lors de l'éjection du Jack sur le pas de tir.

### Filtre de Kalman & Calculs
* `test_baro_pressure_to_altitude` : Vérifie la cohérence de la conversion pression $\rightarrow$ altitude relative (altitude nulle au sol, altitude positive si pression plus basse).
* `test_kalman_reset` : Vérifie la réinitialisation complète des variables de l'estimateur (altitude, vitesse, covariances et altitude max).
* `test_kalman_equations_math` : Valide le fonctionnement des setters de test pour l'injection d'état.
* `test_kalman_apogee_detection` : Simule 5 cycles consécutifs de redescente à Z > 15m et V < -1m/s et vérifie que le trigger d'apogée s'active exactement au 5ème cycle de confirmation (via la fonction isolée `lps22hb_test_apogee_detection_step()`).

### Monitoring Système
* `test_system_timestamp_computation` : Valide le découpage des microsecondes en minutes, secondes et millisecondes.
* `test_system_battery_read_bounds` : Vérifie que la tension de la batterie lue via l'ADC est comprise dans une plage réaliste (entre 0V et 30V).
* `test_system_mcu_temperature_bounds` : Vérifie que la température du MCU lue via l'ADC est cohérente (entre -10°C et 85°C).

---

## 3. Comment compiler et exécuter les tests ?

La configuration utilise l'environnement dédié `[env:pico_test]` dans `platformio.ini`, qui compile les fichiers sources de `src/` tout en excluant `main.cpp` (pour éviter les doublons de fonctions `setup` and `loop`).

### Option A : Compiler uniquement (Vérification statique sans carte connectée)
Pour s'assurer que le code de test compile et se lie correctement avec les librairies du projet :
```powershell
pio test -e pico_test --without-uploading --without-testing
```

### Option B : Exécuter sur la carte physique RP2040
Pour téléverser les tests sur le RP2040 (via votre sonde CMSIS-DAP) et exécuter la suite de tests en récupérant les résultats en direct sur la console :
```powershell
pio test -e pico_test
```

---

## 4. Vérification et Simulation du Filtre de Kalman (Python)

Pour analyser et valider le comportement du filtre de Kalman 1D par rapport aux données théoriques et réelles de vol (sans nécessiter de carte physique connectée), un script de simulation Python est disponible.

Ce script simule précisément le comportement du micrologiciel C++ (boucle à 25 Hz, calculs de prédiction/correction, machine d'état FSM, compteurs de confirmation d'apogée et de touchdown) en utilisant la trajectoire importée de l'Excel [test/stabtraj_v3-5-3.xlsx](file:///c:/Users/paulm/OneDrive/Documents/PlatformIO/Projects/FlightSoftware-Mastodonte/test/stabtraj_v3-5-3.xlsx).

* **Emplacement du script :** [tools/simulate_flight.py](file:///c:/Users/paulm/OneDrive/Documents/PlatformIO/Projects/FlightSoftware-Mastodonte/tools/simulate_flight.py)
* **Guide d'utilisation et d'ajustement du filtre :** Voir le [README de la simulation](file:///c:/Users/paulm/OneDrive/Documents/PlatformIO/Projects/FlightSoftware-Mastodonte/tools/README.md).

