// Map -> level flow: entering each level from the world map must leave that level's
// map in map_data (not the stale world map).
#include <unity.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
static size_t heap_in_use(void) {
    malloc_statistics_t st;
    malloc_zone_statistics(NULL, &st);
    return st.size_in_use;
}
#else
#include <malloc.h>
static size_t heap_in_use(void) { return mallinfo().uordblks; }
#endif
#include "backend.h"
#include "core/globals.h"
#include "keen1_assets.h"
void CVort_engine_prepareEpisodeSpecificFuncPointers(void);

// A level that cannot load must stop the game, not leave the stale map to be played.
static jmp_buf fatal_jmp;
static char fatal_msg[64];
_Noreturn void port_fatal(const char *msg) {
    snprintf(fatal_msg, sizeof fatal_msg, "%s", msg);
    longjmp(fatal_jmp, 1);
}

void setUp(void) { snprintf(game_ext, sizeof(game_ext), "CK1"); }
void tearDown(void) {}

static const AssetFile_T *find(const char *name) {
    for (int i = 0; i < keen1_filesCount; i++)
        if (!strcmp(keen1_files[i].name, name)) return &keen1_files[i];
    return NULL;
}

// Header of the unpacked level (width, height words), independent RLEW decode.
static void header(const AssetFile_T *f, uint16_t *w, uint16_t *h) {
    const uint8_t *p = f->data;
    uint16_t out[3];
    uint32_t in = 4, n = 0;
    while (n < 3 && in + 1 < f->size) {
        uint16_t v = p[in] | p[in + 1] << 8;
        in += 2;
        if (v == 0xFEFE) {
            uint16_t c = p[in] | p[in + 1] << 8, x = p[in + 2] | p[in + 3] << 8;
            in += 4;
            while (c-- && n < 3) out[n++] = x;
        } else
            out[n++] = v;
    }
    *w = out[0];
    *h = out[1];
}

static void setup_episode(void) {
    engine_gameVersion = GAMEVER_KEEN1;
    CVort_engine_prepareEpisodeSpecificFuncPointers();
    TEST_ASSERT_NOT_NULL(CVort_ptr_init_level);
}

static void test_every_level_replaces_the_map(void) {
    setup_episode();
    uint16_t mw, mh;
    header(find("LEVEL80.CK1"), &mw, &mh);
    for (int n = 1; n <= 16; n++) {
        CVort_load_level_data(80);
        TEST_ASSERT_EQUAL_UINT16(mw, map_width_tile);
        char name[16];
        snprintf(name, sizeof name, "LEVEL%02d.CK1", n);
        const AssetFile_T *f = find(name);
        TEST_ASSERT_NOT_NULL_MESSAGE(f, name);
        uint16_t w, h;
        header(f, &w, &h);
        g_entities.num_sprites = 1;
        g_entities.num_bodies = 0;
        CVort_ptr_init_level(n);
        char msg[48];
        snprintf(msg, sizeof msg, "level %d dims", n);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(w, map_width_tile, msg);
        TEST_ASSERT_EQUAL_UINT16_MESSAGE(h, map_height_tile, msg);
        TEST_ASSERT_EQUAL_INT(n, g_game.current_level);
    }
}

// Every entrance (not ship text or teleporter) on the world map must name a level file that exists.
static void test_every_map_entrance_has_a_level_file(void) {
    setup_episode();
    CVort_load_level_data(80);
    for (int i = 0; i < map_width_tile * map_height_tile; i++) {
        uint16_t s = map_data_sprites[i];
        if (!s || s == 0xFF || s == 0x14 || (s & 0x20)) continue; // Keen 1: 0x14 = ship text, 0x2x = teleporter
        uint16_t lv = s & 0xFF;
        char name[16], msg[48];
        snprintf(name, sizeof name, "LEVEL%02u.CK1", lv);
        snprintf(msg, sizeof msg, "sprite 0x%X at %d,%d", s, i % map_width_tile, i / map_width_tile);
        TEST_ASSERT_NOT_NULL_MESSAGE(find(name), msg);
        TEST_ASSERT_TRUE_MESSAGE(lv >= 1 && lv <= 16, msg);
    }
}

static void test_missing_level_is_fatal(void) {
    setup_episode();
    CVort_load_level_data(80);
    fatal_msg[0] = 0;
    if (setjmp(fatal_jmp) == 0)
        CVort_load_level_data(99); // no LEVEL99.CK1
    TEST_ASSERT_EQUAL_STRING("Level 99 failed to load", fatal_msg);
}

// The compressed level buffer must be released: the map loop reloads LEVEL80 every pass.
static void test_level_load_does_not_leak(void) {
    setup_episode();
    CVort_load_level_data(80); // warm any lazy allocations
    size_t before = heap_in_use();
    for (int i = 0; i < 50; i++) {
        CVort_load_level_data(80);
        CVort_load_level_data(1 + i % 16);
    }
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(4096, heap_in_use() - before);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_every_level_replaces_the_map);
    RUN_TEST(test_every_map_entrance_has_a_level_file);
    RUN_TEST(test_missing_level_is_fatal);
    RUN_TEST(test_level_load_does_not_leak);
    return UNITY_END();
}
