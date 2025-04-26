#include "log.h"

#if LOG_ENABLED

#define LOG_FILENAME "/log.txt"  // Nom du fichier de log

// === Initialisation du système de fichiers LittleFS ===
bool log_init(void) {
    debug_println("[LOG] Trying LittleFS.begin()...");

    LittleFSConfig cfg;
    cfg.setAutoFormat(false);
    LittleFS.setConfig(cfg);

    if (!LittleFS.begin()) {
        debug_println("[LOG] Mount failed, formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            debug_println("[LOG] Still failed after format");
            return false;
        }
    }

    debug_println("[LOG] LittleFS mounted.");
    return true;
}

// === Ajoute une ligne de texte brute dans le fichier de log ===
bool log_append(const char* message) {
    // Désactive toutes les interruptions
    uint32_t prev_state = save_and_disable_interrupts();

    File f = LittleFS.open(LOG_FILENAME, "a");
    if (!f) {
        restore_interrupts(prev_state);
        return false;
    }

    f.println(message);
    f.close();

    // Restaure les interruptions à leur état précédent
    restore_interrupts(prev_state);

    return true;
}

// === Ajoute une entrée horodatée dans le log ===
bool log_entry(const char* event) {
    char buffer[256];
    absolute_time_t now = get_absolute_time();
    timestamp_t ts = compute_timestamp(now);
    snprintf(buffer, sizeof(buffer), "[%02lu:%02lu.%03lu.%03lu] %s", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds, event);
    //debug_printf("[LOG ENTRY] %s\n", buffer);
    return log_append(buffer);
}

bool log_entryf(const char* fmt, ...) {
    char event[192];   // Message formaté
    char buffer[256];  // Message final avec timestamp

    va_list args;
    va_start(args, fmt);
    vsnprintf(event, sizeof(event), fmt, args);
    va_end(args);

    absolute_time_t now = get_absolute_time();
    timestamp_t ts = compute_timestamp(now);

    snprintf(buffer, sizeof(buffer), "[%02lu:%02lu.%03lu.%03lu] %s",
             ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds, event);

    //debug_printf("[LOG ENTRY] %s\n", buffer);
    return log_append(buffer);  // <-- Ici, le seul point de sortie réel
}


// === Vérifie si le système de fichiers est presque plein ===
bool log_near_full() {
    FSInfo fs_info;
    LittleFS.info(fs_info);
    size_t free_space = fs_info.totalBytes - fs_info.usedBytes;
    debug_printf("[LOG] FS free space: %lu bytes\n", free_space);
    return free_space < 4096 * 4; // Moins de 4 blocs libres (16kB)
}

// === Supprime complètement le fichier de log ===
void log_clear(void) {
    debug_println("[LOG] Clearing log file");
    LittleFS.remove(LOG_FILENAME);
}

void log_dump(void) {
    static bool serial_was_open = false;

    if (!Serial) {
        Serial.begin(115200);
        unsigned long t0 = millis();
        while (!Serial && (millis() - t0 < 3000));
    } else {
        serial_was_open = true;
    }

    if (!Serial) {
        return; // port toujours indisponible
    }

    File f = LittleFS.open(LOG_FILENAME, "r");
    if (!f) {
        Serial.println("[LOG] No log file to dump");
        if (!serial_was_open) Serial.end();
        return;
    }

    Serial.println();
    Serial.println("[LOG] Dumping log content:");
    while (f.available()) {
        String line = f.readStringUntil('\n');
        Serial.println(line);
    }
    Serial.println();

    f.close();
    if (!serial_was_open) Serial.end();
}

bool log_has_space(void) {
    FSInfo fs_info;
    if (LittleFS.info(fs_info)) {
        // On considère la mémoire "pleine" si plus de 95 % utilisée
        return fs_info.usedBytes < (fs_info.totalBytes * 95 / 100);
    } else {
        // En cas d’erreur, on joue la sécurité
        return false;
    }
}

#endif