// MCP23017 setup sequence (BANK=0 register addresses). See mcp23017.h.
#include "mcp23017.h"

const McpWrite_T mcp_init_seq[] = {
    {0x05, 0x00}, // IOCON if BANK=1 (forces BANK=0); GPINTENB if BANK=0
    {0x0A, 0x00}, // IOCON: BANK=0, sequential addressing, no interrupt tricks
    {0x00, 0xFF}, // IODIRA: all inputs
    {0x01, 0xFF}, // IODIRB: all inputs
    {0x02, 0x00}, // IPOLA: normal polarity
    {0x03, 0x00}, // IPOLB
    {0x04, 0x00}, // GPINTENA: no interrupts
    {0x0C, 0xFF}, // GPPUA: pullups on
    {0x0D, 0xFF}, // GPPUB
};
const int mcp_init_count = sizeof mcp_init_seq / sizeof mcp_init_seq[0];

bool mcp_reinit_due(uint32_t lastInitMs, uint32_t nowMs) {
    return (uint32_t)(nowMs - lastInitMs) >= MCP_REINIT_MS;
}

uint8_t mcp_buttons_from_gpio(uint8_t gpioa, uint8_t gpiob) {
    return (uint8_t)((~gpioa & 0x3F) | ((~gpiob & 0x03) << 6));
}
