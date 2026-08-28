#pragma once

/**
 * Boot splash: firmware version, board and the touch controller that answered
 * on the bus. Shown for a couple of seconds so the hardware can be checked
 * without a serial console.
 */
void statusScreenBootInfo(const char* touch_controller);

/**
 * Short message drawn straight onto the panel (bottom of the round area),
 * used to confirm touch gestures and BOOT presses while testing.
 */
void statusScreenToast(const char* text);

void statusScreenPortal();
void statusScreenConnectFailed();
void statusScreenWifiReset();

/** Saved-network connect animation (call Tick until connect finishes). */
void statusScreenConnectingBegin(const char* ssid);
void statusScreenConnectingTick();
