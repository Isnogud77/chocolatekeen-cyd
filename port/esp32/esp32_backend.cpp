// ESP32 (CYD) backend: ST7789 over SPI DMA, MCP23017 controller over I2C.
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <stdarg.h>
#include "backend.h"
#include "mcp23017.h"

namespace {

constexpr int kSdaPin = 22, kSclPin = 27;
constexpr uint8_t kMcpAddr = 0x20;
constexpr int kStripLines = 20;                 // 10 strips per 200-line frame
constexpr int kStripPixels = 320 * kStripLines; // 12 800 bytes per buffer
constexpr int kTopOffset = 20;                  // center 200 lines on 240

TFT_eSPI tft;
uint16_t *strips[2];
bool i2cFailing = false;
uint32_t fpsCount = 0, fpsSince = 0;

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

} // namespace

extern "C" {

void port_init(void) {
    tft.init();
    tft.setRotation(1);
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
    fpsSince = millis();
}

uint32_t port_millis(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
void port_sleep_ms(uint32_t ms) { delay(ms); }

uint8_t port_read_buttons(void) {
    if (mcp_reinit_due(mcpLastInit, millis())) mcpInit();
    Wire.beginTransmission(kMcpAddr);
    Wire.write(0x12); // GPIOA
    bool ok = Wire.endTransmission(false) == 0 && Wire.requestFrom(kMcpAddr, (uint8_t)1) == 1;
    if (!ok) {
        if (!i2cFailing) port_log("controller: I2C read failed, treating as no input");
        i2cFailing = true;
        return 0;
    }
    if (i2cFailing) port_log("controller: I2C back");
    i2cFailing = false;
    uint8_t raw = Wire.read();
    return (uint8_t)(~raw & 0x3F); // active low; GPA0..5 = PORT_BTN_*
}

void port_present(const uint8_t *src, int stride, const uint16_t pal565[16]) {
    uint16_t pal[16];
    for (int i = 0; i < 16; i++) pal[i] = (uint16_t)((pal565[i] >> 8) | (pal565[i] << 8)); // SPI byte order
    for (int strip = 0; strip < 200 / kStripLines; strip++) {
        uint16_t *dst = strips[strip & 1]; // DMA sends one buffer while we fill the other
        const uint8_t *row = src + strip * kStripLines * stride;
        for (int y = 0; y < kStripLines; y++, row += stride)
            for (int x = 0; x < 320; x++) *dst++ = pal[row[x] & 15];
        tft.pushImageDMA(0, kTopOffset + strip * kStripLines, 320, kStripLines, strips[strip & 1]);
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
