// CYD + MCP23017 controller test
#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>

#define SDA_PIN 22
#define SCL_PIN 27
#define MCP_ADDR 0x20

TFT_eSPI tft;
bool mcpOk = false;
uint16_t lastState = 0xFFFF;

void mcpWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

bool mcpRead16(uint16_t &out) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(0x12); // GPIOA
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MCP_ADDR, 2) != 2) return false;
  uint8_t a = Wire.read(), b = Wire.read();
  out = a | (b << 8);
  return true;
}

void drawPins(uint16_t s) {
  for (int i = 0; i < 16; i++) {
    int x = 10 + (i % 8) * 38, y = 110 + (i / 8) * 60;
    bool low = !(s & (1 << i));
    tft.fillRoundRect(x, y, 34, 40, 4, low ? TFT_GREEN : TFT_DARKGREY);
    tft.setTextColor(TFT_BLACK);
    tft.drawString(String(i < 8 ? "A" : "B") + String(i % 8), x + 6, y + 14, 2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== CYD HW TEST ===");

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_RED);   delay(300);
  tft.fillScreen(TFT_GREEN); delay(300);
  tft.fillScreen(TFT_BLUE);  delay(300);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("CYD HW TEST", 10, 10, 4);
  Serial.println("Display: R/G/B shown");

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Serial.print("I2C scan:");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02X", a); found++; if (a == MCP_ADDR) mcpOk = true; }
  }
  Serial.printf("  (%d found)\n", found);

  if (mcpOk) {
    mcpWrite(0x00, 0xFF); mcpWrite(0x01, 0xFF); // IODIR all input
    mcpWrite(0x0C, 0xFF); mcpWrite(0x0D, 0xFF); // pullups on
    Serial.println("MCP23017 OK");
    tft.drawString("MCP23017 @0x20 OK ", 10, 50, 2);
  } else {
    Serial.println("MCP23017 NOT FOUND");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("MCP23017 NOT FOUND", 10, 50, 2);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Press = green", 10, 75, 2);

}

void loop() {
  if (!mcpOk) { delay(500); return; }
  uint16_t s;
  if (!mcpRead16(s)) { Serial.println("MCP read error"); delay(500); return; }
  if (s != lastState) {
    Serial.printf("[%lu] pins=0x%04X low:", millis(), s);
    for (int i = 0; i < 16; i++) if (!(s & (1 << i))) Serial.printf(" %s%d", i < 8 ? "A" : "B", i % 8);
    Serial.println();
    drawPins(s);
    lastState = s;
  }
  delay(20);
}
