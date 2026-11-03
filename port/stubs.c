// Stubs for upstream symbols this port does not need (yet).
// Each logs its name once, so unexpected calls show up on serial/stderr.
#include "core/globals.h"
#include "platform/platform.h"
#include "backend.h"

#define STUB_ONCE() do { static int seen; if (!seen) { seen = 1; port_log("STUB %s", __func__); } } while (0)

void CK_PlatformApplyDefaultJoystickCalibration(int16_t joystickCtrl[4][3]) { (void)joystickCtrl; }
uint16_t CK_PlatformPreferredPlayer1ControlType(uint16_t currentControlType, int numOfJoysticks) {
    (void)numOfJoysticks;
    return currentControlType;
}
void CK_PlatformSleepMs(unsigned int ms) { port_sleep_ms(ms); }

// No sound in v1.
void CVort_engine_finishCurSound() { STUB_ONCE(); }
void CVort_engine_loadSounds() { STUB_ONCE(); }
void CVort_engine_restoreCurSound() { STUB_ONCE(); }
void CVort_engine_saveCurSound() { STUB_ONCE(); }
void CVort_engine_setCurSound(uint16_t sound) { (void)sound; STUB_ONCE(); }

// Keyboard control only (the controller is mapped to keys).
GameInput_T CVort_engine_getJoystickCtrl(int16_t joy_id) {
    (void)joy_id;
    STUB_ONCE();
    GameInput_T r = {0};
    r.direction = 8;
    return r;
}
GameInput_T CVort_engine_getMouseCtrl() {
    STUB_ONCE();
    GameInput_T r = {0};
    r.direction = 8;
    return r;
}
JoystickPoll_T CVort_engine_pollJoystick(int16_t joystickNum) {
    (void)joystickNum;
    STUB_ONCE();
    JoystickPoll_T r = {0};
    return r;
}
bool CVort_engine_isJoystickInUse() { return false; }

void CVort_engine_handleQuit() { STUB_ONCE(); }
