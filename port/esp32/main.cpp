// ESP32 entry point: run the game in its own task with a large stack.
#include <Arduino.h>
#include "backend.h"

extern "C" void port_game_run(void);

static void gameTask(void *) {
    port_game_run(); // never returns
    vTaskDelete(nullptr);
}

void setup() {
    Serial.begin(115200);
    delay(200);
    port_log("keen-esp32 starting");
    xTaskCreatePinnedToCore(gameTask, "keen", 16384, nullptr, 1, nullptr, 1);
}

void loop() { vTaskDelay(portMAX_DELAY); }
