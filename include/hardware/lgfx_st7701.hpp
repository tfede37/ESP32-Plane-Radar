#pragma once

/**
 * LovyanGFX device for the Waveshare ESP32-S3-Touch-LCD-2.8C:
 * ST7701 480×480 round panel on the ESP32-S3 RGB peripheral, with the vendor
 * init sequence pushed over the 3-wire SPI (SCL/SDA); the panel CS and reset
 * lines hang off the TCA9554 expander and are driven in displayInit().
 */

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

#include "config.h"

/** ST7701 with the panel maker's register set (as shipped by Waveshare). */
class Panel_ST7701_WS28C : public lgfx::Panel_ST7701_Base {
protected:
  const uint8_t* getInitCommands(uint8_t listno) const override {
    static constexpr const uint8_t list0[] = {
        0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
        0xEF, 1, 0x08,

        // Command2 BK0
        0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
        0xC0, 2, 0x3B, 0x00,  // 480 lines
        0xC1, 2, 0x10, 0x0C,
        0xC2, 2, 0x07, 0x0A,
        0xC7, 1, 0x00,
        0xCC, 1, 0x10,
        0xCD, 1, 0x08,

        // Positive / negative voltage gamma
        0xB0, 16, 0x05, 0x12, 0x98, 0x0E, 0x0F, 0x07, 0x07, 0x09,
                  0x09, 0x23, 0x05, 0x52, 0x0F, 0x67, 0x2C, 0x11,
        0xB1, 16, 0x0B, 0x11, 0x97, 0x0C, 0x12, 0x06, 0x06, 0x08,
                  0x08, 0x22, 0x03, 0x51, 0x11, 0x66, 0x2B, 0x0F,

        // Command2 BK1 — power rails
        0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x11,
        0xB0, 1, 0x5D,
        0xB1, 1, 0x3E,
        0xB2, 1, 0x81,
        0xB3, 1, 0x80,
        0xB5, 1, 0x4E,
        0xB7, 1, 0x85,
        0xB8, 1, 0x20,
        0xC1, 1, 0x78,
        0xC2, 1, 0x78,
        0xD0, 1, 0x88,

        // Gate / source timing
        0xE0, 3, 0x00, 0x00, 0x02,
        0xE1, 11, 0x06, 0x30, 0x08, 0x30, 0x05, 0x30, 0x07, 0x30, 0x00, 0x33,
                  0x33,
        0xE2, 12, 0x11, 0x11, 0x33, 0x33, 0xF4, 0x00, 0x00, 0x00, 0xF4, 0x00,
                  0x00, 0x00,
        0xE3, 4, 0x00, 0x00, 0x11, 0x11,
        0xE4, 2, 0x44, 0x44,
        0xE5, 16, 0x0D, 0xF5, 0x30, 0xF0, 0x0F, 0xF7, 0x30, 0xF0,
                  0x09, 0xF1, 0x30, 0xF0, 0x0B, 0xF3, 0x30, 0xF0,
        0xE6, 4, 0x00, 0x00, 0x11, 0x11,
        0xE7, 2, 0x44, 0x44,
        0xE8, 16, 0x0C, 0xF4, 0x30, 0xF0, 0x0E, 0xF6, 0x30, 0xF0,
                  0x08, 0xF0, 0x30, 0xF0, 0x0A, 0xF2, 0x30, 0xF0,
        0xE9, 2, 0x36, 0x01,
        0xEB, 7, 0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40,
        0xED, 16, 0xFF, 0x10, 0xAF, 0x76, 0x54, 0x2B, 0xCF, 0xFF,
                  0xFF, 0xFC, 0xB2, 0x45, 0x67, 0xFA, 0x01, 0xFF,
        0xEF, 6, 0x08, 0x08, 0x08, 0x45, 0x3F, 0x54,

        // Back to the user command set
        0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x00,

        0x11, CMD_INIT_DELAY, 120,  // Sleep out
        0x3A, 1, 0x66,              // RGB666 over the 16-bit bus (R0/B0 unused)
        0x36, 1, 0x00,              // MADCTL: no mirroring, RGB
        0x35, 1, 0x00,              // TE on
        0x20, CMD_INIT_DELAY, 120,  // Inversion off
        0x29, 0,                    // Display on

        0xFF, 0xFF,
    };
    switch (listno) {
      case 0:
        return list0;
      default:
        return nullptr;
    }
  }
};

class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_RGB _bus;
  Panel_ST7701_WS28C _panel;
  lgfx::Light_PWM _light;

public:
  LGFX() {
    {
      auto cfg = _panel.config();
      cfg.memory_width = config::kDisplayWidth;
      cfg.memory_height = config::kDisplayHeight;
      cfg.panel_width = config::kDisplayWidth;
      cfg.panel_height = config::kDisplayHeight;
      cfg.offset_x = config::kDisplayOffsetX;
      cfg.offset_y = config::kDisplayOffsetY;
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
      _panel.config(cfg);
    }
    {
      // CS is on the I/O expander (EXIO3): displayInit() holds it low while
      // LovyanGFX bit-bangs the init sequence, so LGFX itself has no CS pin.
      auto cfg = _panel.config_detail();
      cfg.pin_cs = -1;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.use_psram = 1;
      _panel.config_detail(cfg);
    }
    {
      auto cfg = _bus.config();
      cfg.panel = &_panel;

      // d0..d15 = RGB565 bits 0..15. Swapping the red and blue groups matches
      // the panel's BGR wiring without touching the drawing code.
      const int8_t* low = config::kRgbSwapRedBlue ? config::kRgbPinsRed
                                                  : config::kRgbPinsBlue;
      const int8_t* high = config::kRgbSwapRedBlue ? config::kRgbPinsBlue
                                                   : config::kRgbPinsRed;
      for (int i = 0; i < 5; ++i) {
        cfg.pin_data[i] = low[i];
        cfg.pin_data[11 + i] = high[i];
      }
      for (int i = 0; i < 6; ++i) {
        cfg.pin_data[5 + i] = config::kRgbPinsGreen[i];
      }

      cfg.pin_pclk = static_cast<int>(config::kRgbPinPclk);
      cfg.pin_henable = static_cast<int>(config::kRgbPinDe);
      cfg.pin_vsync = static_cast<int>(config::kRgbPinVsync);
      cfg.pin_hsync = static_cast<int>(config::kRgbPinHsync);

      cfg.freq_write = config::kRgbPixelClockHz;
      cfg.hsync_pulse_width = config::kRgbHsyncPulseWidth;
      cfg.hsync_back_porch = config::kRgbHsyncBackPorch;
      cfg.hsync_front_porch = config::kRgbHsyncFrontPorch;
      cfg.vsync_pulse_width = config::kRgbVsyncPulseWidth;
      cfg.vsync_back_porch = config::kRgbVsyncBackPorch;
      cfg.vsync_front_porch = config::kRgbVsyncFrontPorch;
      cfg.hsync_polarity = 0;
      cfg.vsync_polarity = 0;
      cfg.pclk_active_neg = 1;
      cfg.de_idle_high = 0;
      cfg.pclk_idle_high = 0;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl = static_cast<int>(config::kDisplayPinBacklight);
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
