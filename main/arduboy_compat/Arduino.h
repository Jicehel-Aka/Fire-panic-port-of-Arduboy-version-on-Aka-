// arduboy_compat/Arduino.h — remplace <Arduino.h> (coeur Arduino), non
// disponible sur ESP-IDF. Ne fournit QUE ce dont le code du jeu a reellement
// besoin : PROGMEM (l'ESP32 n'a pas d'espace memoire flash separe a gerer
// comme l'AVR -- macro vide, les tableaux "const" restent lisibles
// normalement), pgm_read_* (simple dereferencement direct), random(),
// __FlashStringHelper/F() (les chaines restent de simples "const char*").
#pragma once
#include <cstdint>
#include <cstdlib>

#define PROGMEM

// BUG TROUVE ET CORRIGE : les macros pgm_read_* forcaient une taille de
// lecture fixe (uint16_t pour pgm_read_word_near, etc.), ce qui casse des
// que le tableau cible est en realite un tableau de POINTEURS (ex:
// Images::Smoke[] = tableau de "const uint8_t*") -- lire 2 octets et les
// reinterpreter en pointeur ne donne jamais une adresse valide. Sur AVR
// (pointeurs 16 bits), "lire un mot" et "lire un pointeur" etaient la meme
// chose ; ce n'est plus vrai sur ESP32 (pointeurs 32 bits). Fix : fonction
// modele qui deduit le bon type DIRECTEMENT depuis le tableau d'origine
// (dereferencement simple), correcte dans les deux cas (nombre ou pointeur)
// sans avoir besoin de deviner une taille a l'avance.
template<typename T>
static inline T pgm_read_any(const T* addr) { return *addr; }

#define pgm_read_byte(addr)       pgm_read_any(addr)
#define pgm_read_word(addr)       pgm_read_any(addr)
#define pgm_read_word_near(addr)  pgm_read_any(addr)
#define pgm_read_dword(addr)      pgm_read_any(addr)
#define pgm_read_ptr(addr)        pgm_read_any(addr)

class __FlashStringHelper;
#define F(s) (s)

static inline long random(long max) { return max > 0 ? (rand() % max) : 0; }
static inline long random(long min, long max) { return max > min ? (min + rand() % (max - min)) : min; }

#define bitSet(v,b)    ((v) |= (1UL << (b)))
#define bitClear(v,b)  ((v) &= ~(1UL << (b)))
#define bitWrite(v,b,x) ((x) ? bitSet(v,b) : bitClear(v,b))
#define bitRead(v,b)   (((v) >> (b)) & 1UL)
