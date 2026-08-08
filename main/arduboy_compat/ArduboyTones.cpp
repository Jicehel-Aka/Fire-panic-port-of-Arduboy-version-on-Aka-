// arduboy_compat/ArduboyTones.cpp
#include "ArduboyTones.h"
#include "gb_audio_track_tone.h"
#include "esp_timer.h"
#include <cstddef>

static gb_audio_track_tone s_track;
static bool s_registered = false;
static bool (*s_outEn)() = nullptr;

static const uint16_t* s_seq = nullptr;      // sequence PROGMEM en cours (ou nullptr)
static size_t s_seqPos = 0;
static uint32_t s_toneStartMs = 0;
static uint16_t s_toneDurMs = 0;
static bool s_seqRepeat = false;

static void ensure_registered() {
    if (s_registered) return;
    s_registered = true;
    extern gb_audio_player g_audio_player;
    g_audio_player.add_track(&s_track, 1.0f);
}

ArduboyTonesExt::ArduboyTonesExt() {}

static void play_one(uint16_t freq, uint16_t dur1024ths) {
    ensure_registered();
    uint16_t durMs = (uint16_t)((uint32_t)dur1024ths * 1000 / 1024);
    if (freq == 0) { s_track.stop_playing(); }
    else if (s_outEn && !s_outEn()) { /* muet : ne joue pas, mais respecte quand meme la duree pour la cadence */ }
    else { s_track.play_tone((float)freq, 0.6f, durMs ? durMs : 200, gb_audio_track_tone::SQUARE); }
    s_toneStartMs = (uint32_t)(esp_timer_get_time() / 1000);
    s_toneDurMs = durMs ? durMs : 200;
}

void ArduboyTonesExt::tone(uint16_t freq, uint16_t dur) {
    s_seq = nullptr;
    play_one(freq, dur ? dur : 1024);
}
void ArduboyTonesExt::tone(uint16_t f1, uint16_t d1, uint16_t f2, uint16_t d2) {
    static uint16_t buf[6];
    buf[0]=f1; buf[1]=d1; buf[2]=f2; buf[3]=d2; buf[4]=TONES_END; buf[5]=0;
    tones(buf);
}
void ArduboyTonesExt::tone(uint16_t f1, uint16_t d1, uint16_t f2, uint16_t d2, uint16_t f3, uint16_t d3) {
    static uint16_t buf[8];
    buf[0]=f1; buf[1]=d1; buf[2]=f2; buf[3]=d2; buf[4]=f3; buf[5]=d3; buf[6]=TONES_END; buf[7]=0;
    tones(buf);
}
void ArduboyTonesExt::tones(const uint16_t* seq) {
    s_seq = seq;
    s_seqPos = 0;
    s_seqRepeat = false;
    if (s_seq) { play_one(s_seq[0], s_seq[1]); s_seqPos = 2; }
}
void ArduboyTonesExt::tonesInRAM(uint16_t* seq) { tones(seq); }
void ArduboyTonesExt::noTone() { s_seq = nullptr; s_track.stop_playing(); }
bool ArduboyTonesExt::playing() { return s_track.is_playing() || s_seq != nullptr; }
void ArduboyTonesExt::volumeMode(uint8_t) { /* un seul niveau de volume gere pour l'instant */ }
void ArduboyTonesExt::setOutputEnabled(bool (*outEn)()) { s_outEn = outEn; }

// A appeler une fois par frame -- fait avancer la sequence en cours quand
// la tonalite actuelle est terminee.
void ArduboyTonesExt::poll() {
    if (!s_seq) return;
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if (now - s_toneStartMs < s_toneDurMs) return;   // tonalite actuelle pas encore finie

    uint16_t freq = s_seq[s_seqPos];
    if (freq == TONES_END) { s_seq = nullptr; return; }
    if (freq == TONES_REPEAT) { s_seqPos = 0; freq = s_seq[0]; }
    uint16_t dur = s_seq[s_seqPos + 1];
    play_one(freq, dur);
    s_seqPos += 2;
}
