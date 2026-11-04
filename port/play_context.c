#include "play_context.h"

static bool s_inPlay, s_inLevel;

void play_context_set(bool inPlay, bool inLevel) {
    s_inPlay = inPlay;
    s_inLevel = inPlay && inLevel;
}
bool play_context_in_play(void) { return s_inPlay; }
bool play_context_in_level(void) { return s_inLevel; }
