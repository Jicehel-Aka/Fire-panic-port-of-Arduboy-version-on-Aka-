// main.cpp — Point d'entree du portage AKA de FirePanic (Press Play On
// Tape, BSD). Portage FIDELE : ecran monochrome 128x64 comme l'original,
// agrandi 2x pour remplir au mieux l'ecran AKA (256x128 sur 320x240).
#include "gb_core.h"
#include "gb_graphics.h"
#include "gb_audio_player.h"
#include "core/input.h"
#include "aka_runtime/aka_runtime.h"
#include "arduboy_compat/Arduboy2.h"
#include "arduboy_compat/ArduboyTones.h"
#include "arduboy_compat/eeprom.h"

#include "game/Game.h"
#include "game/images/Images.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdlib>

gb_core         g_core;
gb_graphics     gfx;
gb_audio_player g_audio_player;

Game game;

static void audio_mix_task(void*) {
    for (;;) {
        g_audio_player.pool();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

extern "C" void app_main(void) {
    Arduboy2Base::earlyHardwareBoot();   // monte la carte SD AVANT akaRuntime.begin() (cf. correctif)
    akaRuntime.begin("firepanic");
    static auto applyVolume = [](uint8_t musicVol, uint8_t /*sfxVol*/) {
        g_audio_player.set_master_volume((uint8_t)((uint16_t)musicVol * 200 / 100));
    };
    akaRuntime.setVolumeChangedCallback(applyVolume);
    // (le volume charge sera applique juste apres la creation de g_audio_player, plus bas)
    eeprom_load();
    static const char* const kControls[] = {
        "CTRL_DPAD", "CTRL_A_B", nullptr
    };
    akaRuntime.setControlsKeys(kControls);
    akaRuntime.setCredits("Fire Panic", "Press Play On Tape", "BSD 3-Clause",
                           "github.com/Press-Play-On-Tape/FirePanic");

    g_audio_player.set_master_volume((uint8_t)((uint16_t)akaRuntime.getMusicVolume() * 200 / 100));
    xTaskCreatePinnedToCore(audio_mix_task, "AudioMixTask", 4096, nullptr, 5, nullptr, 1);

    game.setup();

    // Logo/attribution du jeu affiche en permanence sous la zone de jeu
    // (marge inutilisee de l'ecran AKA) -- element deja present dans le jeu
    // (ecran splash), simplement rendu ici de facon permanente plutot que
    // temporaire. Dessine une seule fois : rien d'autre ne touche cette
    // zone de l'ecran, donc pas besoin de le rafraichir.
    Arduboy2Base::drawStaticBitmapDirect(62, 142, Images::PPOT, 3);

    while (true) {
        input_poll(g_keys);
        if (!akaRuntime.update(g_keys)) { vTaskDelay(pdMS_TO_TICKS(16)); continue; }
        ArduboyTonesExt::poll();
        game.loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
