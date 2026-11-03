// PC speaker player: walks Keen's tone lists (SOUNDS.CK1) in place, one PIT
// divisor per 1/145.6 s tick. No platform code; backends turn divisors into
// sound and call pcspeaker_tick*() from their audio timer/callback.
#ifndef PORT_PCSPEAKER_H
#define PORT_PCSPEAKER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCSPK_PIT_HZ 1193182u
#define PCSPK_TICK_US 6868u // 0x2000 / PCSPK_PIT_HZ seconds

bool pcspeaker_init(const uint8_t *data, uint32_t len); // false: bad magic/size
int pcspeaker_count(void);
bool pcspeaker_play(uint16_t sound); // 1-based; true if started (priority rule)
uint16_t pcspeaker_tick(void);        // divisor for this step, 0 = silent (takes the lock)
uint16_t pcspeaker_tick_locked(void); // same, caller already holds the lock
bool pcspeaker_playing(void);
void pcspeaker_set_muted(bool muted);   // silent, but still advances
void pcspeaker_set_paused(bool paused); // silent, holds position
uint8_t pcspeaker_limiter(void);        // priority of the last started sound
uint32_t pcspeaker_divisor_to_hz(uint16_t divisor); // 0 -> 0, clamped to [20, 20000]

#ifdef __cplusplus
}
#endif
#endif
