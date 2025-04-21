// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : log.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-21
// # Object  : Flash logging using LittleFS on RP2040 (W25Q128)
// -------------------------------------------------------------

#include "log.h"

// === Configuration LittleFS ===
#define FS_BLOCK_SIZE     4096                   // Taille d'un secteur Flash (4kB)
#define FS_BLOCK_COUNT    64                     // Nombre de blocs alloués (64 x 4kB = 256kB)
#define FS_CACHE_SIZE     64
#define FS_LOOKAHEAD_SIZE 16
#define LOG_FILENAME       "/log.txt"           // Fichier principal

#define LOG_FLASH_OFFSET   (2 * 1024 * 1024 - FS_BLOCK_SIZE * FS_BLOCK_COUNT) // À la fin des 2MB de Flash
#define FLASH_TARGET_OFFSET LOG_FLASH_OFFSET

extern uint8_t __flash_binary_end; // Marqueur de fin du binaire flash
static const uint8_t* flash_storage = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);

// === Contexte LittleFS ===
static struct lfs_config cfg;
static lfs_t lfs;

// Prototypes internes
static int lfs_read_cb (const struct lfs_config*, lfs_block_t, lfs_off_t, void*, lfs_size_t);
static int lfs_prog_cb (const struct lfs_config*, lfs_block_t, lfs_off_t, const void*, lfs_size_t);
static int lfs_erase_cb(const struct lfs_config*, lfs_block_t);
static int lfs_sync_cb (const struct lfs_config*);

// === Initialisation du système de fichiers LittleFS ===
bool log_init(void) {
    cfg.read  = lfs_read_cb;
    cfg.prog  = lfs_prog_cb;
    cfg.erase = lfs_erase_cb;
    cfg.sync  = lfs_sync_cb;

    cfg.read_size = 256;
    cfg.prog_size = 256;
    cfg.block_size = FS_BLOCK_SIZE;
    cfg.block_count = FS_BLOCK_COUNT;
    cfg.block_cycles = 500;
    cfg.cache_size = FS_CACHE_SIZE;
    cfg.lookahead_size = FS_LOOKAHEAD_SIZE;
    cfg.context = NULL;

    int err = lfs_mount(&lfs, &cfg);
    if (err) {
        lfs_format(&lfs, &cfg);
        err = lfs_mount(&lfs, &cfg);
        if (err) return false;
    }
    return true;
}

// === Ajoute une ligne de log ===
bool log_append(const char* message) {
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, LOG_FILENAME, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) < 0) return false;
    lfs_file_write(&lfs, &file, message, strlen(message));
    lfs_file_write(&lfs, &file, "\n", 1);
    lfs_file_close(&lfs, &file);
    return true;
}

// === Réinitialise le fichier de log ===
void log_clear(void) {
    lfs_remove(&lfs, LOG_FILENAME);
}

// === Callbacks pour accès flash physique ===

static int lfs_read_cb(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    const uint8_t* addr = flash_storage + block * FS_BLOCK_SIZE + off;
    memcpy(buffer, addr, size);
    return 0;
}

static int lfs_prog_cb(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(FLASH_TARGET_OFFSET + block * FS_BLOCK_SIZE + off, (const uint8_t*)buffer, size);
    restore_interrupts(ints);
    return 0;
}

static int lfs_erase_cb(const struct lfs_config *c, lfs_block_t block) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET + block * FS_BLOCK_SIZE, FS_BLOCK_SIZE);
    restore_interrupts(ints);
    return 0;
}

static int lfs_sync_cb(const struct lfs_config *c) {
    return 0;
}
