#include "hardware/display_font.h"

#include "config.h"
#include "hardware/display.h"
#include "ui/ui_scale.h"

extern "C" {
#if defined(BOARD_UI_FONT_30PX)
// 480×480 panels embed data/ui_font_30.vlw (see platformio.ini).
extern const uint8_t _binary_data_ui_font_30_vlw_start[] asm(
    "_binary_data_ui_font_30_vlw_start");
extern const uint8_t _binary_data_ui_font_30_vlw_end[] asm(
    "_binary_data_ui_font_30_vlw_end");
#define UI_FONT_VLW_START _binary_data_ui_font_30_vlw_start
#define UI_FONT_VLW_END _binary_data_ui_font_30_vlw_end
#else
extern const uint8_t _binary_data_ui_font_vlw_start[] asm(
    "_binary_data_ui_font_vlw_start");
extern const uint8_t _binary_data_ui_font_vlw_end[] asm("_binary_data_ui_font_vlw_end");
#define UI_FONT_VLW_START _binary_data_ui_font_vlw_start
#define UI_FONT_VLW_END _binary_data_ui_font_vlw_end
#endif
}

namespace {

bool s_vlw_loaded = false;

const uint8_t* vlwData() { return UI_FONT_VLW_START; }

size_t vlwDataLen() {
  return static_cast<size_t>(UI_FONT_VLW_END - UI_FONT_VLW_START);
}

bool vlwActiveOn(const lgfx::LGFXBase& gfx) {
  const lgfx::IFont* font = gfx.getFont();
  return font != nullptr && font->getType() == lgfx::IFont::font_type_t::ft_vlw;
}

}  // namespace

bool displayFontInit() {
  s_vlw_loaded = vlwDataLen() > 0 &&
                 tft.loadFont(vlwData(), lgfx::IFont::font_type_t::ft_vlw);
  if (!s_vlw_loaded) {
    Serial.println("Smooth font load failed — using bitmap fallback");
  }
  return s_vlw_loaded;
}

bool displayFontIsSmooth() { return s_vlw_loaded; }

bool displayFontEnsureLoaded(lgfx::LGFXBase& gfx) {
  if (!s_vlw_loaded) {
    return false;
  }
  if (vlwActiveOn(gfx)) {
    return true;
  }
  return gfx.loadFont(vlwData(), lgfx::IFont::font_type_t::ft_vlw);
}

void displayFontSetSmoothSize(lgfx::LGFXBase& gfx, float size) {
  // Sizes are expressed in 240 px / 15 px-font design units.
  gfx.setTextSize(size * ui::kSmoothFontScale);
}

void displayFontSetBitmap(lgfx::LGFXBase& gfx, const lgfx::GFXfont* font) {
  gfx.setFont(font);
  gfx.setTextSize(ui::kTextScale);
}
