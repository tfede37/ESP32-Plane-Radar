#pragma once

/**
 * Waveshare ESP32-S3-Touch-LCD-2.8C
 *   Display : ST7701(S), 2.8" round, 480×480, 16-bit RGB parallel
 *             (+ 3-wire SPI for the init sequence)
 *   Touch   : GT911, I2C
 *   Expander: TCA9554PWR @ 0x20 — carries LCD reset/CS and touch reset
 *   MCU     : ESP32-S3R8 (8 MB octal PSRAM, 16 MB flash)
 *
 * Selected with -DBOARD_WAVESHARE_LCD_28C (env:waveshare-28c).
 * Pin map: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.8C
 */

#include <cstdint>

#include <driver/gpio.h>

#define BOARD_DISPLAY_ST7701_RGB 1
#define BOARD_TOUCH_GT911 1
#define BOARD_HAS_IO_EXPANDER 1
/** Twice the pixel density of the 1.28" layout: use the 30 px smooth font. */
#define BOARD_UI_FONT_30PX 1
#define BOARD_NAME "Waveshare ESP32-S3-Touch-LCD-2.8C"

namespace config {

// --- BOOT button (active LOW, hardware pull-up) ---
constexpr gpio_num_t kBootPin = GPIO_NUM_0;

// --- Display: ST7701 2.8" round 480×480 (RGB565 parallel) ---
constexpr int kDisplayWidth = 480;
constexpr int kDisplayHeight = 480;
constexpr int kDisplayOffsetX = 0;
constexpr int kDisplayOffsetY = 0;

/** 3-wire SPI used only for the panel init sequence (CS lives on the expander). */
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_2;  // LCD_SCL
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_1;  // LCD_SDA
constexpr gpio_num_t kDisplayPinBacklight = GPIO_NUM_6;  // PWM backlight (active HIGH)

// RGB timing signals.
constexpr gpio_num_t kRgbPinPclk = GPIO_NUM_41;
constexpr gpio_num_t kRgbPinDe = GPIO_NUM_40;
constexpr gpio_num_t kRgbPinVsync = GPIO_NUM_39;
constexpr gpio_num_t kRgbPinHsync = GPIO_NUM_38;

// RGB data lines, least significant bit first. R0/B0 are not connected on this
// board (the panel runs RGB666 with the two LSBs unused), so five red / five
// blue and six green lines carry an RGB565 frame.
constexpr int8_t kRgbPinsRed[5] = {46, 3, 8, 18, 17};        // R1..R5
constexpr int8_t kRgbPinsGreen[6] = {14, 13, 12, 11, 10, 9};  // G0..G5
constexpr int8_t kRgbPinsBlue[5] = {5, 45, 48, 47, 21};       // B1..B5

/** This panel expects BGR order: send red on the blue lines and vice versa.
 *  Flip to false if red and blue come out swapped on your unit. */
constexpr bool kRgbSwapRedBlue = true;

// Panel timing (16 MHz dot clock keeps the PSRAM framebuffer tear-free).
constexpr uint32_t kRgbPixelClockHz = 16000000;
constexpr int8_t kRgbHsyncPulseWidth = 8;
constexpr int8_t kRgbHsyncBackPorch = 10;
constexpr int8_t kRgbHsyncFrontPorch = 50;
constexpr int8_t kRgbVsyncPulseWidth = 3;
constexpr int8_t kRgbVsyncBackPorch = 8;
constexpr int8_t kRgbVsyncFrontPorch = 8;

/** Colors are corrected in the bus wiring above, so no palette swap is needed. */
constexpr bool kDisplayRgbOrder = false;
constexpr bool kDisplayInvert = false;
/** Backlight duty (0-255): the 2.8" panel is dimmer than the 1.28" one. */
constexpr uint8_t kDisplayBrightness = 200;
/** Pixel size of the embedded VLW smooth font (data/ui_font_30.vlw). */
constexpr float kEmbeddedVlwFontPx = 30.0f;

// --- Shared I2C bus: touch + expander (+ IMU / RTC, unused here) ---
constexpr gpio_num_t kTouchPinSda = GPIO_NUM_15;
constexpr gpio_num_t kTouchPinScl = GPIO_NUM_7;
constexpr uint32_t kI2cFrequencyHz = 400000;

// --- TCA9554PWR I/O expander (EXIO1..EXIO8 = port bits 0..7) ---
constexpr uint8_t kIoExpanderI2cAddr = 0x20;
constexpr uint8_t kExioLcdReset = 0;    // EXIO1
constexpr uint8_t kExioTouchReset = 1;  // EXIO2
constexpr uint8_t kExioLcdCs = 2;       // EXIO3
constexpr uint8_t kExioSdCs = 3;        // EXIO4
constexpr uint8_t kExioBuzzer = 7;      // EXIO8 (kept quiet at boot)

// --- Touchscreen: GT911 (I2C), reset via EXIO2 ---
constexpr gpio_num_t kTouchPinInt = GPIO_NUM_16;
/** 0x5D with INT held low during reset (0x14 is the alternate address). */
constexpr uint8_t kTouchI2cAddr = 0x5D;

}  // namespace config
