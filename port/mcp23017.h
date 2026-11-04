// MCP23017 controller expander: full register setup, re-applied often.
//
// A hot-plug of the controller cable can write garbage into the chip's
// registers (seen: IPOLA = IOCON = 0x7A, inverting five inputs) without any
// I2C error, and the chip keeps it because it stays powered from the CYD.
// So we set every register the input path depends on, and do it again
// every MCP_REINIT_MS.
#ifndef PORT_MCP23017_H
#define PORT_MCP23017_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint8_t reg, val; } McpWrite_T;

extern const McpWrite_T mcp_init_seq[];
extern const int mcp_init_count;

#define MCP_REINIT_MS 1000u

// GPIOA/GPIOB (active low) -> PORT_BTN_* mask: GPA0..5 = bits 0..5, GPB0 = Start (6), GPB1 = Menu (7).
uint8_t mcp_buttons_from_gpio(uint8_t gpioa, uint8_t gpiob);

bool mcp_reinit_due(uint32_t lastInitMs, uint32_t nowMs);

#ifdef __cplusplus
}
#endif
#endif
