#pragma once

/**
 * Waveshare ESP32-S3-Touch-LCD-1.28
 *   Display : GC9A01, 1.28" round, 240×240, SPI
 *   Touch   : CST816, I2C
 *
 * Selected by default (no BOARD_* build flag) — see include/config.h.
 */

#include <cstdint>

#include <driver/gpio.h>

#define BOARD_DISPLAY_GC9A01_SPI 1
#define BOARD_TOUCH_CST816 1
#define BOARD_NAME "Waveshare ESP32-S3-Touch-LCD-1.28"

namespace config {

// --- BOOT button (active LOW) ---
// On the S3 board GPIO9 is the display CS, so the user button is the
// on-board BOOT button on GPIO0 (active LOW, has hardware pull-up).
constexpr gpio_num_t kBootPin = GPIO_NUM_0;

// --- Display: GC9A01 1.28" round 240×240 (SPI) ---
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_14;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_9;
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_8;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_11;  // display SDA
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_10;  // display SCL
constexpr gpio_num_t kDisplayPinBacklight = GPIO_NUM_2;  // PWM backlight (active HIGH)

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 240;

// Panel alignment: this board's visible window sits a few rows high, clipping
// the top. Shift all output down by this many px (0 = no shift).
constexpr int kDisplayOffsetX = 0;
constexpr int kDisplayOffsetY = 0;

constexpr uint32_t kDisplaySpiWriteHz = 40000000;
// GC9A01 modules often need invert + BGR for correct black/green output
constexpr bool kDisplayInvert = true;
constexpr bool kDisplayRgbOrder = true;
/** Backlight duty (0-255): ~16% — cooler, low power, easy on the eyes. */
constexpr uint8_t kDisplayBrightness = 40;
/** Pixel size of the embedded VLW smooth font (data/ui_font.vlw). */
constexpr float kEmbeddedVlwFontPx = 15.0f;

// --- Touchscreen: CST816 (I2C) on the Waveshare board ---
constexpr gpio_num_t kTouchPinSda = GPIO_NUM_6;
constexpr gpio_num_t kTouchPinScl = GPIO_NUM_7;
constexpr gpio_num_t kTouchPinInt = GPIO_NUM_5;
constexpr gpio_num_t kTouchPinRst = GPIO_NUM_13;
constexpr uint8_t kTouchI2cAddr = 0x15;
constexpr uint32_t kI2cFrequencyHz = 400000;

}  // namespace config
