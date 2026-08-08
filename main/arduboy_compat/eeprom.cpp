// arduboy_compat/eeprom.cpp
#include "eeprom.h"
#include "aka_runtime/aka_runtime.h"
#include <cstdio>

uint8_t g_eeprom_buf[1024];

void eeprom_load() {
    memset(g_eeprom_buf, 0xFF, sizeof(g_eeprom_buf));   // 0xFF = etat "vierge" habituel de l'EEPROM
    char path[96];
    snprintf(path, sizeof path, "%s/eeprom.dat", akaRuntime.gamePath());
    FILE* f = fopen(path, "rb");
    if (f) { fread(g_eeprom_buf, 1, sizeof(g_eeprom_buf), f); fclose(f); }
}

void eeprom_save() {
    char path[96];
    snprintf(path, sizeof path, "%s/eeprom.dat", akaRuntime.gamePath());
    FILE* f = fopen(path, "wb");
    if (f) { fwrite(g_eeprom_buf, 1, sizeof(g_eeprom_buf), f); fclose(f); }
}
