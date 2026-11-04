# Full game flow — rulings and deferred follow-ups

From the SDD ledger of plan docs/superpowers/plans/2026-11-04-full-game-flow.md (2026-11-04).

## Rulings
- Task 1: Ruling: checkLevel compares words-1 — upstream CRLE_expandSwapped decodes the size header's high word as data, so the last (unused) sprite-plane word is never produced; port stays faithful to upstream — cost if wrong: a real last-word bug would be masked in tests
- Task 3: Ruling: picker events go through a small queue drained only when key_scane bit 7 is clear (upstream consumed) — reconciles "set once on press edge" (each event is set exactly once) with the two-event Backspace+letter need — cost if wrong: one extra frame of latency per letter
- Task 6: Ruling: accept renderer change outside play (redraw only changed tiles, like upstream ATR) — spec requires text drawn once to stay visible; in play unchanged — cost if wrong: stale tiles on non-play screens
- Task 6: Ruling: attract cycle must not show About ID / Ordering Info — spec lists them out of scope (hidden) — enters fix round
- Task 6: Ruling: quit prompt text changed to a title-only question (no "Dos") — cabinet has no DOS; b1 = yes/title, b2 = no — enters fix round
- Task 6: Ruling: accept KEEN_AUTO=M also tapping Down until 25 s — level 1 entrance needs it on the map — cost if wrong: none (host debug aid)
- Task 6: Ruling: char-wait button mapping (input_map.c:56) untested — upstream flag is static with getter only; carry to Task 8: add a test seam + unit test there (Task 8 works on char-wait anyway) — cost if wrong: prompt-stuck regression undetected
- Task 7: Ruling: fix the data, not the runtime — convert data/KEEN1 STORYTXT/ENDTEXT/PREVIEWS to CRLF, regenerate assets, add .gitattributes marking data/** binary, drop port_text_crlf + menus.c patch; keep test_text as a data check — less code, faithful bytes — cost if wrong: if some file was LF in the original release, it now gets CRs
- Task 9: Ruling: b1 key event becomes Enter (0x1C) in menu and char-wait contexts; held keys unchanged (Enter+Y+Space); patched quit prompts (gameplay.c) accept Enter as yes; save-confirm (menus.c:757) to be handled in part 3 — one event per press avoids leaking a second key into the next prompt — cost if wrong: an unpatched Y-only prompt can't be confirmed with b1
- Task 9: parked — Sound (Y/N) (gameplay.c:401) and save confirm (menus.c:757) lose b1 "yes" — Ruling: Sound prompt only via F2, unreachable on cabinet; save confirm belongs to part 3 (carry: accept Enter as yes there) — cost if wrong: a reachable Y-only prompt can't be confirmed with b1
- Task 9: Ruling: treat upstream's blocking translate_key(0) loop as "waiting for input" (ESP32-PORT flag set during the loop), so buttons map as in menus there (b1 Enter event); do NOT emit events from jump/pogo during play — stale events would auto-dismiss later prompts — cost if wrong: other any-key waits not using translate_key(0) still stuck

## Parked
- Task 9: parked — Sound (Y/N) (gameplay.c:401) and save confirm (menus.c:757) lose b1 "yes" — Ruling: Sound prompt only via F2, unreachable on cabinet; save confirm belongs to part 3 (carry: accept Enter as yes there) — cost if wrong: a reachable Y-only prompt can't be confirmed with b1

## Deferred minors
- Task 1: minor (deferred): checkLevel should assert oracle word count before decrement
- Task 1: minor (deferred): test_all_levels_load failure message lacks level name
- Task 2: minor (deferred): magic indices 5/3 for pulse_active instead of PORT_BTN_* derived
- Task 2: minor (deferred): several buttons pressed in one poll → only last key_scane event survives (untested)
- Task 2: minor (deferred): missing tests: Y/N release in menu, no b2 long-press Esc in menu context, context switch while held
- Task 3: minor (deferred): Right with nothing tentative still types A (spurious letter)
- Task 3: minor (deferred): trans_map mapping of 0x0E/0x39/0x1C through CVort_translate_key untested
- Task 3: minor (deferred): held[] list duplicates SC_* enum in input_map.c
- Task 3: minor (deferred): FIFO overflow (8) can drop Enter → picker never deactivates (practically unreachable, one-line guard)
- Task 4: minor (deferred): raw asset arrays not explicitly aligned; do_image_file_decomp reads Uint32 length → possible unaligned load on Xtensa (verify on device in Task 9; fix: aligned(4) in assetdump)
- Task 4: minor (deferred): image decode not bounded by output size (build-time assets)
- Task 4: minor (deferred): scrollText test covers only page origin 0x3000; test_render not extended
- Task 5: minor (deferred): play_context forces inLevel = inPlay && inLevel (hides mis-set calls)
- Task 6: minor (deferred): stray whitespace hunk menus.c:303 without marker
- Task 6: minor (deferred): spritesDrawn / render_image_mode_off cache invalidations untested
- Task 6: minor (deferred): unreachable default branch in port menu switch
- Task 6: minor (deferred): host dumpFrame getenv twice per present
- Task 6: minor (deferred): b1 menu selection only verified by host frames
- Task 7: minor (deferred): KEEN_AUTO S/H/C take precedence over M/R by strchr order
- Task 8: minor (deferred): yorp statue in-level messages (CVort1_inlevel_message) not exercised on host — same wait path; add to device check
- Task 8: minor (deferred): char-wait test restores flag in-body, not tearDown
- Task 8: minor (deferred): host D/X script logic duplicated
- Task 9: minor (deferred): docs/controls.md does not name the status-screen key (Space)
- Task 9: minor (deferred): b1 Enter event in menu context not audited for unconsumed events leaking into the next prompt
- Task 9: minor (deferred): quit patch Enter path has no unit test
- Task 9: leak fix re-review: all addressed (commits 78f0663..4eac7b3); minor (deferred): fread-failure branch skips heap report
- Task 9: status fix re-review: all addressed (commits 4eac7b3..a6202b7); minor (deferred): host_backend comment indentation
- Final: minor (deferred): unaligned flash reads — arrays 4-aligned in ELF; optional aligned(4) in assetdump
- Final: minor (deferred): ending (FINALE, level 81, win → game_over) never run on device
- Final: minor (deferred): docs/controls.md: b2 hold opens quit prompt (not a menu); status key Space not named
- Final: minor (deferred): port_fatal hangs forever; consider esp_restart after ~10 s (product call)
- Final: fixed I1 picker Enter drop — test_full_queue_still_ends_with_enter RED→GREEN, suite 119/119; re-review clean; minor (deferred): overwrite may drop the currently shown letter / empty name possible
