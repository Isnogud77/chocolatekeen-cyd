// Whether the game is in play (vs menus/text screens) and inside a level
// (vs the map). Input mapping and screen mode depend on it.
#ifndef PORT_PLAY_CONTEXT_H
#define PORT_PLAY_CONTEXT_H

#include <stdbool.h>

void play_context_set(bool inPlay, bool inLevel);
bool play_context_in_play(void);
bool play_context_in_level(void);

#endif
