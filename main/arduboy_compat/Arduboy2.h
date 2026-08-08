// arduboy_compat/Arduboy2.h — Reimplementation de l'API reelle Arduboy2Base
// (bibliotheque externe attendue par le jeu, PAS le code du jeu lui-meme)
// utilisee par FirePanic (Press Play On Tape, BSD). Portage FIDELE : ecran
// monochrome 128x64 (noir/blanc), agrandi a l'affichage pour remplir l'AKA
// sans deformer.
//
// IMPORTANT : le tampon sBuffer utilise le VRAI format Arduboy (1024 octets,
// 128 colonnes x 8 "pages" de 8 pixels verticaux chacune, 1 bit/pixel) --
// pas un format simplifie -- car le jeu (Arduboy2Ext.cpp, SplashScreenState)
// accede directement a sBuffer par calcul d'adresse (page*WIDTH + x).
#pragma once
#include <cstdint>
#include <cstddef>

#ifndef SCALE_FACTOR
#define SCALE_FACTOR 1
#endif

#define WHITE 1
#define BLACK 0

// WIDTH/HEIGHT existent aussi en macros globales (pas seulement en membres
// de classe) -- le jeu les utilise nues dans plusieurs fichiers.
#define WIDTH  128
#define HEIGHT 64

#define A_BUTTON     0x08
#define B_BUTTON     0x04
#define UP_BUTTON    0x80
#define DOWN_BUTTON  0x10
#define LEFT_BUTTON  0x20
#define RIGHT_BUTTON 0x40

struct Point { int16_t x, y; };
struct Rect  { int16_t x, y; uint8_t width, height; };

// "enabled" est utilise SANS parentheses (arduboy.audio.enabled, passe
// comme pointeur de fonction) -- doit donc etre une fonction STATIQUE.
class ArduboyAudio {
public:
    void begin() {}
    static bool enabled() { return true; }
    void on() {}
    void off() {}
    void toggle() {}
    void saveOnOff() {}
};

class Arduboy2Base {
public:
    // WIDTH/HEIGHT : macros globales uniquement (voir plus haut) -- PAS de
    // membres statiques du meme nom ici, ça creerait un conflit textuel
    // avec le preprocesseur (la macro remplacerait "WIDTH" y compris dans
    // cette declaration).

    ArduboyAudio audio;

    // Tampon reel (format Arduboy natif), accessible directement comme sur
    // le vrai materiel -- certains jeux y ecrivent en direct pour des
    // effets speciaux (ex: SplashScreenState).
    uint8_t sBuffer[(WIDTH * HEIGHT) / 8];

    // Etats de boutons et compteur de frame -- accessibles directement
    // (Arduboy2Ext.cpp, code du jeu, y accede comme s'il en heritait).
    uint8_t  currentButtonState = 0;
    uint8_t  previousButtonState = 0;
    uint16_t frameCount = 0;

    // Appelable depuis main.cpp AVANT que le jeu ne cree son Arduboy2Ext
    // (monte la carte SD tot, pour qu'akaRuntime.begin() puisse lire/ecrire
    // ses fichiers) -- ne depend d'aucune instance, idempotente.
    static void earlyHardwareBoot();

    void boot();
    bool nextFrame();
    void display();
    void display(bool clear);

    void pollButtons();
    bool pressedButtons(uint8_t buttons) const;
    uint8_t justPressedButtons() const;
    bool justPressed(uint8_t button) const;
    bool pressed(uint8_t button) const;
    void clearButtonState();
    bool systemButtons();

    void setFrameRate(uint8_t fps);
    uint8_t getFrameRate() const;
    void initRandomSeed();
    bool everyXFrames(uint8_t frames);

    void exitToBootloader();
    void setRGBled(uint8_t r, uint8_t g, uint8_t b);
    bool flashlight();

    void clear();
    void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
    void drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
    void drawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t color = WHITE);
    void drawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t color = WHITE);
    void drawPixel(int16_t x, int16_t y, uint8_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE);

    // Dessine un bitmap UNE FOIS, directement sur l'ecran physique, en
    // dehors de la zone de jeu -- pour un element permanent (logo, etc.)
    // qui reste affiche independamment du rendu du jeu.
    static void drawStaticBitmapDirect(int16_t physX, int16_t physY, const uint8_t* bitmap, uint8_t scale);

    static bool collide(Point p, Rect r);
    static bool collide(Rect a, Rect b);

    void setCursor(int16_t x, int16_t y);
    void setTextSize(uint8_t s);
    void print(const char* s);
    void print(int v);
    void print(char c);
    void println(const char* s);
};

class Arduboy2 : public Arduboy2Base {
public:
};

// --- SpritesB : dessin de sprites avec masque, decouvert a l'usage (non ---
// --- present dans l'inventaire initial). Format Arduboy standard : chaque
// bitmap [W,H] puis donnees 1bpp par page ; le masque (meme dimensions)
// indique quels pixels sont opaques (1) vs transparents (0).
class SpritesB {
public:
    // Dessine en respectant le masque externe fourni separement.
    static void drawExternalMask(int16_t x, int16_t y, const uint8_t* bitmap,
                                  const uint8_t* mask, uint8_t frame, uint8_t maskFrame);
    // Le bitmap contient sa propre transparence (0x00 = transparent).
    static void drawSelfMasked(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame);
    // Ecrase completement (aucune transparence).
    static void drawOverwrite(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame);
    // Efface (dessine en "eteint" partout ou le bitmap est oppose).
    static void drawErase(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame);
};
