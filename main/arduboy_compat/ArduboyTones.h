// arduboy_compat/ArduboyTones.h — Reimplementation de l'API reelle
// ArduboyTonesExt (bibliotheque externe) utilisee par FirePanic. Sequenceur
// de tonalites (frequence/duree), sur gb_audio_track_tone.
#pragma once
#include <cstdint>

#define TONES_END    0x8000
#define TONES_REPEAT 0x8001

class ArduboyTonesExt {
public:
    ArduboyTonesExt();
    static void tone(uint16_t freq, uint16_t dur = 0);
    static void tone(uint16_t freq1, uint16_t dur1, uint16_t freq2, uint16_t dur2);
    static void tone(uint16_t freq1, uint16_t dur1, uint16_t freq2, uint16_t dur2,
                      uint16_t freq3, uint16_t dur3);
    static void tones(const uint16_t* tones);
    static void tonesInRAM(uint16_t* tones);
    static void noTone();
    static bool playing();
    static void volumeMode(uint8_t mode);
    static void setOutputEnabled(bool (*outEn)());

    // Appele une fois par frame (depuis Arduboy2Base::nextFrame() ou le
    // main.cpp) pour faire avancer la sequence en cours.
    static void poll();
};
