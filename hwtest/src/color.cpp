// Color test: cycle inversion x RGB/BGR, 4 s each
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft;

void show(int n, bool inv, bool bgr) {
  tft.invertDisplay(inv);
  tft.writecommand(0x36);               // MADCTL, rotation 1 = MX|MV
  tft.writedata(0x60 | (bgr ? 0x08 : 0x00));
  int w = 320 / 4;
  tft.fillRect(0,     0, w, 240, TFT_RED);
  tft.fillRect(w,     0, w, 240, TFT_GREEN);
  tft.fillRect(2 * w, 0, w, 240, TFT_BLUE);
  tft.fillRect(3 * w, 0, w, 240, TFT_WHITE);
  tft.fillRect(0, 180, 320, 60, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(String(n) + ": inv " + (inv ? "ON" : "OFF") + ", " + (bgr ? "BGR" : "RGB"), 10, 195, 4);
  Serial.printf("Variant %d: inv=%d bgr=%d\n", n, inv, bgr);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
}

void loop() {
  show(1, true,  false); delay(4000);
  show(2, true,  true);  delay(4000);
  show(3, false, false); delay(4000);
  show(4, false, true);  delay(4000);
}
