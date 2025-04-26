
#include "log.h"

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
    File f = LittleFS.open(LOG_FILENAME, "a");
    if (!f) return false;
    f.println(message);
    f.close();
    return true;
}

// === Ajoute une entrée horodatée dans le log ===
bool log_entry(const char* event) {
    char buffer[256];
    absolute_time_t now = get_absolute_time();
    timestamp_t ts = compute_timestamp(now);
    snprintf(buffer, sizeof(buffer), "[%02lu:%02lu.%03lu.%03lu] %s", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds, event);
    debug_printf("[LOG ENTRY] %s\n", buffer);
    return log_append(buffer);
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

// === Affiche le contenu du log sur la sortie série ===
void log_dump(void) {
    File f = LittleFS.open(LOG_FILENAME, "r");
    if (!f) {
        debug_println("[LOG] No log file to dump");
        return;
    }

    debug_println("[LOG] Dumping log content:");
    while (f.available()) {
        String line = f.readStringUntil('\n');
        debug_println(line.c_str());
    }

    f.close();
}