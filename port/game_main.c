// Boots the game the way upstream CVort_engine_loadKeen() does, then runs
// level 1 in a loop (upstream CVort_main calls port_level_loop at its end).
#include "core/globals.h"
#include "engine/engine_timing.h"
#include "keen1_assets.h"
#include "backend.h"
#include "input_map.h"
#include "render.h"

// Upstream defines these without a header declaration.
void CVort_engine_prepareEpisodeSpecificFuncPointers(void);
uint16_t CVort_private_continue_game(void);

void port_level_loop(void) {
    input_init_scancodes();
    for (;;) {
        // New-game state, as in upstream CVort_do_intro_and_menu (Keen 1).
        g_game.resuming_saved_game = 0;
        keen_gp.lives = 4;
        keen_gp.ammo = 0;
        memset(keen_gp.stuff, 0, sizeof(keen_gp.stuff));
        memset(keen_gp.targets, 0, sizeof(keen_gp.targets));
        memset(keen_gp.levels, 0, sizeof(keen_gp.levels));
        g_game.extra_life_pts = 0;
        g_entities.sprites[0].pos_x = 0;
        g_entities.sprites[0].pos_y = 0;
        g_game.anim_speed = 3;

        uint16_t result = CVort_draw_level(1);
        port_log("level 1 ended: %u (finished %d, quit %d, lives %d, keen type %d)", result, g_game.level_finished, g_game.quit_to_title, keen_gp.lives, g_entities.sprites[0].type_);
        port_report_heap("level end");
    }
}

void port_game_run(void) {
    port_init();
    if (!render_init())
        port_fatal("Out of memory for playfield");

    exeImage = (uint8_t *)keen1_exeImage; // read-only, in flash
    engine_gameVersion = GAMEVER_KEEN1;
    CVort_engine_prepareEpisodeSpecificFuncPointers();
    if (!anim_frame_tiles[0])
        port_fatal("Out of memory for tile animation");
    CVort_ptr_engine_processEXE();

    // Tail of upstream CVort_engine_loadKeen (non-demo path, EGA timing).
    CVort_ptr_engine_getTicks = CVort_private_engine_getTicks;
    CVort_ptr_engine_setTicks = CVort_private_engine_setTicks;
    CVort_ptr_engine_getTicksSync = CVort_private_engine_getTicksSync;
    CVort_ptr_engine_setTicksSync = CVort_private_engine_setTicksSync;
    CVort_ptr_continue_game = CVort_private_continue_game;
    g_game.demo_status = DEMO_OFF;
    draw_func = NULL;
    engine_doResetMomentToDelay = true;
    engine_arguments.calc.scaledRefreshRate = ENGINE_EGA_REFRESHRATE_SCALED;
    engine_refreshTimeInMicroSec = ((int64_t)1000000) * ENGINE_EGAVGA_REFRESHRATE_SCALE_FACTOR / engine_arguments.calc.scaledRefreshRate;
    engine_sdlMicroTicksDelayOffset = 0;
    engine_gameTicksDelayOffset = 0;
    engine_gameTicksStart = 0;
    CVort_private_engine_setTicks(0);

    g_game.want_sound = 1; // upstream sets this from its config menu, which the port lacks

    port_report_heap("boot");
    CVort_main(); // ends in port_level_loop(), never returns
}
