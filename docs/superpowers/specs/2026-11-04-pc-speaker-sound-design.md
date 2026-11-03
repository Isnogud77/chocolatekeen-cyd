# PC Speaker Sound — Design

Date: 2026-11-04
Builds on: `2026-11-03-chocolatekeen-port-design.md` (level 1 milestone, merged)

## Goal

Keen 1 sound effects on the CYD speaker, sounding like the original PC speaker
(hard square wave). Same learning goal as the port: small steps, the reasoning
explained.

**Done when:** in level 1 on the device, jump, shot (or the empty-gun click),
death and exit sounds play with the right pitch and timing; fps stays ~24 and
free heap drops by no more than 1 KB. The host build plays the same sounds.

**Out of scope:** volume control, smoother waveforms (DAC), music, a sound
on/off menu.

## Constraints and facts

- Speaker: GPIO 26 → onboard amplifier.
- `SOUNDS.CK1`: 8 898 bytes, magic `SND\0`, sound count (uint16 LE) at
  offset 8 = **60**. Entry *i* (1-based) is 16 bytes at `16*i`: offset of its
  tone list (uint16 LE) at +0, priority (uint8) at +2, name at +4.
- A tone list is a run of uint16 LE PIT divisors, one per 1/145.6 s
  (0x2000 / 1 193 182 Hz ≈ 6.87 ms), ended by `0xFFFF`. 0 = silence.
  Tone frequency = 1 193 182 / divisor.
- Upstream pre-renders all sounds to 48 kHz 16-bit PCM (~2.9 MB for Keen 1).
  That cannot fit; we play tone lists directly instead.
- Upstream rules kept exactly (`engine_audio.c`):
  - Game sound numbers are 1-based.
  - A sound starts only if nothing plays or its priority ≥ `sound_limiter`;
    starting sets `sound_limiter` to its priority.
  - Nothing starts while `g_game.sound_disabled`.
  - When `g_game.want_sound` is 0, output is silent but the sound still
    advances.
  - `finishCurSound` waits (via `CVort_engine_shortSleep`) until playback ends.
- `g_game.want_sound` is 0 at boot; upstream sets it from its config menu,
  which the port lacks. **The port sets it to 1 at boot.**

## Approach

Hardware PWM (LEDC) makes the square wave; a 145.6 Hz `esp_timer` advances the
tone list and sets the PWM frequency. Exact timing, independent of the ~41 ms
game frame, near-zero CPU and RAM. Rejected: advancing in the game loop (tones
would jump in 41 ms steps); DAC sample output (more code, not needed for an
authentic sound).

## Structure

```
port/pcspeaker.c/.h    platform-neutral player
port/audio_glue.c      upstream sound API (replaces the stubs)
port/esp32/            + esp_timer 145.6 Hz → pcspeaker_tick() → LEDC tone / off
port/host/             + SDL audio callback, same tick → square-wave samples
assets/                + SOUNDS.CK1 raw in flash, read in place (no malloc)
```

`pcspeaker.h`:

- `bool pcspeaker_init(const uint8_t *data, uint32_t len)`: validates the
  magic and count, keeps a pointer into flash. False → sound stays off.
- `void pcspeaker_play(uint16_t sound)`: 1-based, applies the priority rule.
- `uint16_t pcspeaker_tick(void)`: returns the divisor for this 6.87 ms step
  (0 = silent, also when nothing plays or when muted) and advances.
- `bool pcspeaker_playing(void)`
- `void pcspeaker_set_muted(bool muted)`, `void pcspeaker_set_paused(bool paused)`
- `int pcspeaker_count(void)`

The player keeps its own copy of the priority limiter and mirrors it into
upstream's `sound_limiter`.

`audio_glue.c` implements `CVort_engine_loadSounds`, `setCurSound`,
`finishCurSound`, `saveCurSound` (pause) and `restoreCurSound` (resume) on top
of the player, with upstream's `sound_disabled` and `want_sound` checks. The
`STUB_ONCE` versions in `port/stubs.c` are removed.

Backend: on the ESP32, a divisor *d* > 0 → LEDC tone at 1 193 182 / *d* Hz with
50 % duty, clamped to the LEDC range; 0 → output off. On the host, the SDL
callback runs the same tick every 6.87 ms of samples and writes ± amplitude.

## Concurrency

The timer (ESP32: esp_timer task; host: SDL audio thread) and the game task
share the player state. Every public player function runs under a short lock:
`portENTER_CRITICAL` on the ESP32, `SDL_LockAudioDevice` on the host, provided
by the backend through `port_audio_lock()` / `port_audio_unlock()`.

## Errors

- `SOUNDS.CK1` missing or bad header → silent game, one log line.
- Sound number outside 1..count → ignored, logged once.

## Testing

Host unit tests with the real `SOUNDS.CK1`:

- `pcspeaker_count() == 60`.
- After `play(n)`, `tick()` returns that sound's tone words in order, then 0,
  and `playing` becomes false.
- Priority: a lower priority does not interrupt; an equal or higher one does.
- Muted: `tick` returns 0 while the position still advances.
- Out-of-range sound numbers are ignored.

On the device, the user checks the four sounds; serial shows fps and heap
unchanged.
