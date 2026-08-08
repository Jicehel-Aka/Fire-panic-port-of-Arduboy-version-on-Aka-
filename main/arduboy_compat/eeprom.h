// arduboy_compat/eeprom.h — remplace <avr/eeprom.h> (specifique AVR).
// L'ESP32 n'a pas d'EEPROM separee comme l'AVR ; on simule le meme modele
// (adresse = simple decalage d'octet) avec un tampon charge/sauvegarde
// depuis un fichier sur la carte SD -- meme principe que GameCookie
// (Kong-II), adapte au style d'acces "adresse-pointeur" attendu ici.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

typedef uint8_t byte;

extern uint8_t g_eeprom_buf[1024];
void eeprom_load();
void eeprom_save();

static inline uint8_t eeprom_read_byte(const uint8_t* addr) {
    return g_eeprom_buf[(size_t)addr];
}
static inline void eeprom_update_byte(uint8_t* addr, uint8_t val) {
    g_eeprom_buf[(size_t)addr] = val;
    eeprom_save();
}
static inline uint16_t eeprom_read_word(const uint16_t* addr) {
    uint16_t v; memcpy(&v, &g_eeprom_buf[(size_t)addr], 2); return v;
}
static inline void eeprom_update_word(uint16_t* addr, uint16_t val) {
    memcpy(&g_eeprom_buf[(size_t)addr], &val, 2);
    eeprom_save();
}
static inline void eeprom_read_block(void* dst, const void* addr, size_t n) {
    memcpy(dst, &g_eeprom_buf[(size_t)addr], n);
}
static inline void eeprom_update_block(const void* src, void* addr, size_t n) {
    memcpy(&g_eeprom_buf[(size_t)addr], src, n);
    eeprom_save();
}
