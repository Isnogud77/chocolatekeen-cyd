// Boots the game the way upstream CVort_engine_loadKeen() does, then runs
// upstream's front end (CVort_main -> CVort_main_loop: logo, title, menu,
// world map, levels, ending), which never returns.
#include "core/globals.h"
#include "engine/engine_timing.h"
#include "keen1_assets.h"
#include "backend.h"
#include "render.h"
#include "screen_layout.h"

// Upstream defines these without a header declaration.
void CVort_engine_prepareEpisodeSpecificFuncPointers(void);
uint16_t CVort_private_continue_game(void);

#ifndef KEEN_PORTRAIT
#define KEEN_PORTRAIT 0 // build envs *_portrait set 1
#endif

void port_game_run(void) {
    screen_layout_set_portrait(KEEN_PORTRAIT); // before port_init: sets the screen size
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
    CVort_main(); // ends in CVort_main_loop(), never returns
}
