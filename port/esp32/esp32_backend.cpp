// ESP32 (CYD) backend: ST7789 over SPI DMA, MCP23017 controller over I2C.
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <hal/dac_ll.h>
#include <stdarg.h>
#include "backend.h"
#include "mcp23017.h"
#include "pcspeaker.h"

namespace {

constexpr int kSdaPin = 22, kSclPin = 27;
constexpr uint8_t kMcpAddr = 0x20;
constexpr int kStripLines = 20;                 // 10 strips per 200-line frame
constexpr int kStripPixels = 320 * kStripLines; // 12 800 bytes per buffer
constexpr int kSpeakerPin = 26;        // DAC_CHAN_1
// Volume: the DAC square wave swings 0 .. amplitude (full would be 255).
// The CYD amplifier is very loud, so we keep it well below full.
// Silence = 0, like the PC speaker at rest (no DC jump when a tone starts).
// The faint "flimmern" between sounds comes from the board (also there with
// plain PWM), not from this code.
constexpr uint8_t kSpeakerAmplitude = 16;
constexpr uint32_t kSpeakerMaxHz = 5000; // caps the toggle interrupt at 10 kHz
hw_timer_t *toneTimer;
volatile bool toneHigh;
bool speakerRunning = false; // both timers started

TFT_eSPI tft;
uint16_t *strips[2];
bool i2cFailing = false;
uint32_t fpsCount = 0, fpsSince = 0;
portMUX_TYPE audioMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t mcpLastInit = 0;

// Write the full register setup (see mcp23017.h for why it is repeated).
void mcpInit() {
    for (int i = 0; i < mcp_init_count; i++) {
        Wire.beginTransmission(kMcpAddr);
        Wire.write(mcp_init_seq[i].reg);
        Wire.write(mcp_init_seq[i].val);
        Wire.endTransmission();
    }
    mcpLastInit = millis();
}

// Hardware timer interrupt at twice the tone frequency: flip the DAC level.
void IRAM_ATTR onToneHalfPeriod() {
    toneHigh = !toneHigh;
    dac_ll_update_output_value(DAC_CHAN_1, toneHigh ? kSpeakerAmplitude : 0);
}

void setTone(uint32_t hz) {
    timerStop(toneTimer);
    if (!hz) {
        dac_ll_update_output_value(DAC_CHAN_1, 0); // silence = 0 V
        return;
    }
    if (hz > kSpeakerMaxHz) hz = kSpeakerMaxHz;
    toneHigh = false; // wave starts from the resting 0 level
    timerAlarm(toneTimer, 500000u / hz, true, 0); // half period in us (1 MHz timer)
    timerWrite(toneTimer, 0);
    timerStart(toneTimer);
}

// esp_timer task, every 6.868 ms: advance the PC speaker, retune the tone.
void speakerTick(void *) {
    static uint32_t lastHz = 0;
    port_audio_lock();
    uint32_t hz = pcspeaker_divisor_to_hz(pcspeaker_tick_locked());
    port_audio_unlock(); // no timer calls inside the critical section
    if (hz != lastHz) {
        setTone(hz);
        lastHz = hz;
    }
}

void startSpeaker() {
    dacWrite(kSpeakerPin, 0); // set up the DAC pad; later writes go to the register
    toneTimer = timerBegin(1000000);
    if (!toneTimer) {
        port_log("sound: tone timer failed, playing silently");
        return;
    }
    timerAttachInterrupt(toneTimer, onToneHalfPeriod);
    timerStop(toneTimer);
    esp_timer_create_args_t args = {};
    args.callback = speakerTick;
    args.name = "pcspeaker";
    esp_timer_handle_t timer;
    if (esp_timer_create(&args, &timer) != ESP_OK || esp_timer_start_periodic(timer, PCSPK_TICK_US) != ESP_OK) {
        port_log("sound: timer start failed, playing silently");
        return;
    }
    speakerRunning = true;
}

} // namespace

extern "C" {

void port_init(void) {
    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase(); // partition full or from another layout: start clean
        nvsErr = nvs_flash_init();
    }
    if (nvsErr != ESP_OK) port_log("store: NVS init failed (%d), saving disabled", (int)nvsErr);
    tft.init();
    tft.setRotation(screen_layout_portrait() ? 0 : 1); // 0 = upright in the Galagino cabinet
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(false); // we write big-endian RGB565 ourselves
    tft.initDMA();
    for (auto &s : strips) {
        s = (uint16_t *)heap_caps_malloc(kStripPixels * 2, MALLOC_CAP_DMA);
        if (!s) port_fatal("Out of DMA memory");
    }
    tft.startWrite(); // keep the SPI bus for DMA; only the display uses it

    Wire.begin(kSdaPin, kSclPin, 400000);
    mcpInit();
    startSpeaker();
    fpsSince = millis();
}

// Store: NVS namespace "keen", one blob per name; handle opened per call.
int port_store_load(const char *name, uint8_t *buf, int max) {
    nvs_handle_t h;
    if (nvs_open("keen", NVS_READONLY, &h) != ESP_OK) return -1;
    size_t len = (size_t)max;
    esp_err_t err = nvs_get_blob(h, name, buf, &len);
    nvs_close(h);
    return err == ESP_OK ? (int)len : -1;
}

bool port_store_save(const char *name, const uint8_t *buf, int len) {
    nvs_handle_t h;
    if (nvs_open("keen", NVS_READWRITE, &h) != ESP_OK) return false;
    bool ok = nvs_set_blob(h, name, buf, (size_t)len) == ESP_OK && nvs_commit(h) == ESP_OK;
    nvs_close(h);
    return ok;
}

void port_store_erase(const char *name) {
    nvs_handle_t h;
    if (nvs_open("keen", NVS_READWRITE, &h) != ESP_OK) return;
    if (nvs_erase_key(h, name) == ESP_OK) nvs_commit(h);
    nvs_close(h);
}

bool port_audio_available(void) { return speakerRunning; }
void port_audio_lock(void) { portENTER_CRITICAL(&audioMux); }
void port_audio_unlock(void) { portEXIT_CRITICAL(&audioMux); }

uint32_t port_millis(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
void port_sleep_ms(uint32_t ms) { delay(ms); }

uint8_t port_read_buttons(void) {
    if (mcp_reinit_due(mcpLastInit, millis())) mcpInit();
    Wire.beginTransmission(kMcpAddr);
    Wire.write(0x12); // GPIOA
    bool ok = Wire.endTransmission(false) == 0 && Wire.requestFrom(kMcpAddr, (uint8_t)2) == 2; // GPIOA + GPIOB (sequential addressing)
    if (!ok) {
        if (!i2cFailing) port_log("controller: I2C read failed, treating as no input");
        i2cFailing = true;
        return 0;
    }
    if (i2cFailing) port_log("controller: I2C back");
    i2cFailing = false;
    uint8_t gpioa = Wire.read();
    uint8_t gpiob = Wire.read();
    return mcp_buttons_from_gpio(gpioa, gpiob);
}

void port_present(const ScreenLayout_T *layout, const uint8_t *src, int stride, const uint16_t pal565[16]) {
    uint16_t pal[16];
    for (int i = 0; i < 16; i++) pal[i] = (uint16_t)((pal565[i] >> 8) | (pal565[i] << 8)); // SPI byte order
    static int lastMode = -1;
    if ((int)layout->mode != lastMode) { // bars differ per mode: clear once
        tft.dmaWait();
        tft.fillScreen(TFT_BLACK);
        lastMode = layout->mode;
    }
    for (int y0 = 0, n = 0; y0 < layout->outH; y0 += kStripLines, n++) {
        int lines = layout->outH - y0 < kStripLines ? layout->outH - y0 : kStripLines; // last strip may be short
        uint16_t *dst = strips[n & 1]; // DMA sends one buffer while we fill the other
        for (int y = 0; y < lines; y++)
            screen_layout_row(layout, src, stride, y0 + y, pal, dst + y * layout->outW);
        tft.pushImageDMA(0, layout->top + y0, layout->outW, lines, strips[n & 1]);
    }
    if (++fpsCount, millis() - fpsSince >= 5000) {
        port_log("fps %.1f", fpsCount * 1000.0f / (millis() - fpsSince));
        fpsCount = 0;
        fpsSince = millis();
    }
}

void port_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

void port_report_heap(const char *where) {
    port_log("heap @%s: free %u, largest block %u", where,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

void port_fatal(const char *msg) {
    port_log("FATAL: %s", msg);
    tft.dmaWait();
    tft.endWrite();
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("FATAL", 10, 10, 2);
    tft.drawString(msg, 10, 30, 2);
    for (;;) delay(1000);
}

} // extern "C"
