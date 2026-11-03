// ESP32 entry point. The game task comes in Task 6.
#include <Arduino.h>
#include "backend.h"

extern "C" uint16_t CVort_draw_level(uint16_t levelnum);
extern "C" void CVort_engine_prepareEpisodeSpecificFuncPointers(void);

void setup() {
    Serial.begin(115200);
    port_report_heap("boot");
    // Keeps game code linked so the size report is real; never true.
    if (millis() == 0xFFFFFFFFu) {
        CVort_engine_prepareEpisodeSpecificFuncPointers();
        CVort_draw_level(1);
    }
}

void loop() { delay(1000); }
