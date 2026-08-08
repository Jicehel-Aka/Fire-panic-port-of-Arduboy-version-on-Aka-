// arduboy_compat/Arduboy2.cpp — Implementation, sur gb_graphics, de l'API
// Arduboy2Base reellement utilisee par FirePanic. Tampon au format Arduboy
// natif (128x64, 1bpp, pages de 8 lignes), agrandi a l'affichage.
#include "Arduboy2.h"
#include "aka_runtime/aka_runtime.h"
#include "core/input.h"
#include "gb_core.h"
#include "gb_graphics.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

extern gb_core g_core;
extern gb_graphics gfx;

static constexpr int PHYS_X = 32, PHYS_Y = 8, PHYS_SCALE = 2;

static uint16_t* s_decodeBuf = nullptr;
Arduboy2Base* g_arduboySpriteTarget = nullptr;   // instance ciblee par SpritesB (declare ici, utilise des boot())
static uint8_t   s_fps = 60;
static uint32_t  s_lastFrameMs = 0;
static int16_t   s_cursorX = 0, s_cursorY = 0;
static uint8_t   s_textSize = 1;

// Unique instance globale -- le jeu declare "Arduboy2Ext arduboy;" ailleurs,
// mais present() etc. ont besoin d'un point d'entree statique pour sBuffer ;
// on passe donc par "this" partout (methodes non-statiques), sBuffer vit
// directement dans l'objet Arduboy2Base comme sur le vrai materiel.

static void ensure_decode_buf() {
    if (!s_decodeBuf) s_decodeBuf = (uint16_t*)heap_caps_malloc(256*128*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
}

// Partie materielle (montage SD, ecran) -- ne depend d'aucune instance,
// appelable directement depuis main.cpp AVANT que le jeu ne cree son propre
// objet Arduboy2Ext. Idempotente (le 2e appel, fait par le jeu lui-meme
// dans Game::setup() via arduboy.boot(), ne refait rien).
static bool s_hwBooted = false;
void Arduboy2Base::earlyHardwareBoot() {
    if (s_hwBooted) return;
    s_hwBooted = true;
    ensure_decode_buf();
    g_core.init();
    gfx.set_backlight_percent(80);
    gfx.set_refresh_rate(60);
}

void Arduboy2Base::boot() {
    // BUG TROUVE ET CORRIGE : arduboy.boot() (qui monte la carte SD via
    // g_core.init()) n'etait appele qu'a l'interieur de Game::setup(), donc
    // APRES akaRuntime.begin() -- toutes les lectures/ecritures de reglages
    // (langue, volume) a ce moment-la echouaient silencieusement, carte pas
    // encore montee. Fix : la partie materielle (earlyHardwareBoot) est
    // appelee plus tot depuis main.cpp ; ce qui reste ici (idempotent grace
    // a earlyHardwareBoot) est inoffensif si rappele par le jeu lui-meme.
    earlyHardwareBoot();
    memset(sBuffer, 0, sizeof(sBuffer));
    g_arduboySpriteTarget = this;   // SpritesB dessine dans CETTE instance (une seule existe dans le jeu)
}

bool Arduboy2Base::nextFrame() {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t frame_ms = 1000u / s_fps;
    if (s_lastFrameMs != 0 && (now - s_lastFrameMs) < frame_ms) return false;
    s_lastFrameMs = now;
    input_poll(g_keys);
    ++frameCount;
    return true;
}

// Dessine un bitmap (format Arduboy standard, 1bpp) directement sur l'ecran
// physique, EN DEHORS de la zone de jeu -- pour un element permanent (ex:
// logo/attribution) qui n'est jamais efface par le rendu du jeu lui-meme
// (qui ne touche que sa propre zone 256x128). A appeler UNE FOIS (le
// contenu reste affiche indefiniment, rien ne l'efface).
void Arduboy2Base::drawStaticBitmapDirect(int16_t physX, int16_t physY, const uint8_t* bitmap, uint8_t scale) {
    uint8_t w = bitmap[0], h = bitmap[1];
    const uint8_t* px = bitmap + 2;
    int pages = (h + 7) / 8;
    gfx.setColor(gfx.makeColor(255, 255, 255));
    for (int page = 0; page < pages; ++page) {
        for (int col = 0; col < w; ++col) {
            uint8_t b = px[page*w + col];
            for (int bit = 0; bit < 8; ++bit) {
                int py = page*8 + bit;
                if (py >= h) break;
                if (!(b & (1 << bit))) continue;
                gfx.fillRect(physX + col*scale, physY + py*scale, scale, scale);
            }
        }
    }
    gfx.update();
}

void Arduboy2Base::display() { display(false); }
void Arduboy2Base::display(bool doClear) {
    ensure_decode_buf();
    if (!s_decodeBuf) return;
    uint16_t white = gfx.makeColor(255,255,255), black = gfx.makeColor(0,0,0);
    for (int y = 0; y < HEIGHT; ++y) {
        int page = y >> 3, bit = y & 7;
        for (int x = 0; x < WIDTH; ++x) {
            uint8_t b = sBuffer[page * WIDTH + x];
            uint16_t c = (b & (1 << bit)) ? white : black;
            int px = x*PHYS_SCALE, py = y*PHYS_SCALE;
            s_decodeBuf[py*256+px] = c; s_decodeBuf[py*256+px+1] = c;
            s_decodeBuf[(py+1)*256+px] = c; s_decodeBuf[(py+1)*256+px+1] = c;
        }
    }
    gfx.drawImage(PHYS_X, PHYS_Y, s_decodeBuf, 256, 128);
    gfx.update();
    if (doClear) memset(sBuffer, 0, sizeof(sBuffer));
}

void Arduboy2Base::pollButtons() {
    input_poll(g_keys);
    previousButtonState = currentButtonState;
    uint8_t m = 0;
    if (g_keys.A) m |= A_BUTTON;
    if (g_keys.B) m |= B_BUTTON;
    if (g_keys.up) m |= UP_BUTTON;
    if (g_keys.down) m |= DOWN_BUTTON;
    if (g_keys.left) m |= LEFT_BUTTON;
    if (g_keys.right) m |= RIGHT_BUTTON;
    currentButtonState = m;
}

bool Arduboy2Base::pressedButtons(uint8_t buttons) const { return (currentButtonState & buttons) == buttons; }
uint8_t Arduboy2Base::justPressedButtons() const { return (~previousButtonState) & currentButtonState; }
bool Arduboy2Base::justPressed(uint8_t button) const { return justPressedButtons() & button; }
bool Arduboy2Base::pressed(uint8_t button) const { return currentButtonState & button; }
void Arduboy2Base::clearButtonState() { currentButtonState = previousButtonState = 0; }
bool Arduboy2Base::systemButtons() { return false; }

void Arduboy2Base::setFrameRate(uint8_t fps) { s_fps = fps ? fps : 60; }
uint8_t Arduboy2Base::getFrameRate() const { return s_fps; }
void Arduboy2Base::initRandomSeed() { srand((unsigned)(esp_timer_get_time())); }
bool Arduboy2Base::everyXFrames(uint8_t frames) { return frames ? (frameCount % frames) == 0 : false; }

void Arduboy2Base::exitToBootloader() { akaRuntime.returnToLoader(); }
void Arduboy2Base::setRGBled(uint8_t, uint8_t, uint8_t) {}
bool Arduboy2Base::flashlight() { return false; }

void Arduboy2Base::clear() { memset(sBuffer, 0, sizeof(sBuffer)); }

// Acces bit a bit direct dans le tampon au format Arduboy natif.
static inline void set_px(uint8_t* buf, int16_t x, int16_t y, bool on) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    int page = y >> 3, bit = y & 7;
    uint8_t& b = buf[page * WIDTH + x];
    if (on) b |= (1 << bit); else b &= ~(1 << bit);
}

void Arduboy2Base::fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
    for (int16_t yy = y; yy < y+h; ++yy)
        for (int16_t xx = x; xx < x+w; ++xx)
            set_px(sBuffer, xx, yy, color != 0);
}
void Arduboy2Base::drawRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
    drawFastHLine(x, y, w, color); drawFastHLine(x, y+h-1, w, color);
    drawFastVLine(x, y, h, color); drawFastVLine(x+w-1, y, h, color);
}
void Arduboy2Base::drawFastVLine(int16_t x, int16_t y, uint8_t h, uint8_t color) { fillRect(x, y, 1, h, color); }
void Arduboy2Base::drawFastHLine(int16_t x, int16_t y, uint8_t w, uint8_t color) { fillRect(x, y, w, 1, color); }
void Arduboy2Base::drawPixel(int16_t x, int16_t y, uint8_t color) { set_px(sBuffer, x, y, color != 0); }

void Arduboy2Base::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t w, uint8_t h, uint8_t color) {
    int pages = (h + 7) / 8;
    for (int page = 0; page < pages; ++page) {
        for (int col = 0; col < w; ++col) {
            uint8_t b = bitmap[page*w + col];
            for (int bit = 0; bit < 8; ++bit) {
                int py = page*8 + bit;
                if (py >= h) break;
                if (!(b & (1 << bit))) continue;
                set_px(sBuffer, x+col, y+py, color != 0);
            }
        }
    }
}

bool Arduboy2Base::collide(Point p, Rect r) {
    return p.x >= r.x && p.x < r.x+r.width && p.y >= r.y && p.y < r.y+r.height;
}
bool Arduboy2Base::collide(Rect a, Rect b) {
    return a.x < b.x+b.width && a.x+a.width > b.x && a.y < b.y+b.height && a.y+a.height > b.y;
}

void Arduboy2Base::setCursor(int16_t x, int16_t y) { s_cursorX = x; s_cursorY = y; }
void Arduboy2Base::setTextSize(uint8_t s) { s_textSize = s ? s : 1; }
void Arduboy2Base::print(char c) {
    if (c != ' ') fillRect(s_cursorX, s_cursorY, 3*s_textSize, 5*s_textSize, WHITE);
    s_cursorX += 4*s_textSize;
}
void Arduboy2Base::print(const char* s) { while (*s) print(*s++); }
void Arduboy2Base::print(int v) { char buf[12]; snprintf(buf, sizeof buf, "%d", v); print(buf); }
void Arduboy2Base::println(const char* s) { print(s); s_cursorX = 0; s_cursorY += 6*s_textSize+1; }

// --- SpritesB --------------------------------------------------------
// NOTE : sans instance globale exposee publiquement (le jeu cree son propre
// "Arduboy2Ext arduboy;"), SpritesB doit dessiner dans CE MEME tampon. On
// recupere l'unique instance via un pointeur global fixe par Arduboy2Base
// au demarrage (simple : un seul Arduboy2Ext existe dans tout le jeu).
static void sprite_draw(int16_t x, int16_t y, const uint8_t* bitmap, const uint8_t* mask,
                         uint8_t frame, uint8_t maskFrame, int mode) {
    if (!g_arduboySpriteTarget || !bitmap) return;
    uint8_t w = bitmap[0], h = bitmap[1];
    const uint8_t* px = bitmap + 2 + (size_t)frame * (((h+7)/8) * w);
    // BUG TROUVE ET CORRIGE : contrairement au bitmap, le tableau de masque
    // n'a PAS d'en-tete [largeur,hauteur] -- ce sont directement les
    // donnees brutes, aux memes dimensions que le bitmap associe. Sauter 2
    // octets ici (comme pour le bitmap) decalait systematiquement toute la
    // lecture du masque, causant les contours incomplets/mal alignes
    // observes a l'usage.
    const uint8_t* mpx = mask ? mask + (size_t)maskFrame * (((h+7)/8) * w) : nullptr;
    int pages = (h + 7) / 8;
    for (int page = 0; page < pages; ++page) {
        for (int col = 0; col < w; ++col) {
            uint8_t b = px[page*w + col];
            uint8_t m = mpx ? mpx[page*w + col] : 0xFF;
            for (int bit = 0; bit < 8; ++bit) {
                int py = page*8 + bit;
                if (py >= h) break;
                bool srcOn = b & (1 << bit);
                bool maskOn = m & (1 << bit);
                switch (mode) {
                    case 0: // drawExternalMask : n'affecte que les pixels non masques
                        if (maskOn) set_px(g_arduboySpriteTarget->sBuffer, x+col, y+py, srcOn);
                        break;
                    case 1: // drawSelfMasked : le bitmap est sa propre transparence
                        if (srcOn) set_px(g_arduboySpriteTarget->sBuffer, x+col, y+py, true);
                        break;
                    case 2: // drawOverwrite : ecrase tout, aucune transparence
                        set_px(g_arduboySpriteTarget->sBuffer, x+col, y+py, srcOn);
                        break;
                    case 3: // drawErase : eteint la ou le bitmap est actif
                        if (srcOn) set_px(g_arduboySpriteTarget->sBuffer, x+col, y+py, false);
                        break;
                }
            }
        }
    }
}

void SpritesB::drawExternalMask(int16_t x, int16_t y, const uint8_t* bitmap, const uint8_t* mask, uint8_t frame, uint8_t maskFrame) {
    sprite_draw(x, y, bitmap, mask, frame, maskFrame, 0);
}
void SpritesB::drawSelfMasked(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame) {
    sprite_draw(x, y, bitmap, nullptr, frame, 0, 1);
}
void SpritesB::drawOverwrite(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame) {
    sprite_draw(x, y, bitmap, nullptr, frame, 0, 2);
}
void SpritesB::drawErase(int16_t x, int16_t y, const uint8_t* bitmap, uint8_t frame) {
    sprite_draw(x, y, bitmap, nullptr, frame, 0, 3);
}
