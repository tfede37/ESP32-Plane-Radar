#include "hardware/display.h"

#include <Arduino.h>

#include "config.h"
#include "hardware/display_font.h"
#include "hardware/io_expander.h"

LGFX tft;

namespace {

#if defined(BOARD_DISPLAY_ST7701_RGB)
/**
 * The ST7701 reset and CS lines sit on the TCA9554. Pulse reset, then hold CS
 * low so LovyanGFX can bit-bang the whole init sequence in one 3-wire SPI
 * transaction (it never toggles CS itself: pin_cs is -1).
 */
void panelBusPrepare() {
  hardware::i2cBegin();
  hardware::expander::begin();

  hardware::expander::write(config::kExioSdCs, true);  // keep the TF card idle
  hardware::expander::write(config::kExioLcdCs, true);
  hardware::expander::write(config::kExioLcdReset, true);
  delay(10);
  hardware::expander::write(config::kExioLcdReset, false);
  delay(20);
  hardware::expander::write(config::kExioLcdReset, true);
  delay(120);

  hardware::expander::write(config::kExioLcdCs, false);
}

void panelBusRelease() { hardware::expander::write(config::kExioLcdCs, true); }
#else
void panelBusPrepare() {}
void panelBusRelease() {}
#endif

}  // namespace

void displayInit() {
  panelBusPrepare();
  tft.init();
  panelBusRelease();

  tft.setRotation(0);
  tft.setBrightness(config::kDisplayBrightness);
  tft.setTextWrap(false);
  displayFontInit();
}
