#include "ui/weather_display.h"

#include <WiFi.h>
#include <lgfx/v1/lgfx_fonts.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "services/weather.h"
#include "ui/radar_theme.h"
#include "ui/ui_scale.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {

namespace {

using services::weather::Condition;

constexpr int kCx = config::kDisplayWidth / 2;

/** Layout is authored for 240 px and scaled to the active panel. */
using ui::scaled;
using ui::scaledf;

/** Boards with a BGR panel swap R/B (see initPalette in radar_display): feed
 *  colors through here so logical RGB renders correctly. */
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  if (config::kDisplayRgbOrder) {
    return tft.color565(b, g, r);
  }
  return tft.color565(r, g, b);
}

// Match the radar background exactly: it uses the unswapped color path, so the
// same constants render as the same near-black tone (no bluish wash).
uint16_t bgColor() {
  return tft.color565(radar::kBgR, radar::kBgG, radar::kBgB);
}
uint16_t textColor() { return rgb(255, 255, 255); }

// ---- weather icons (centered on cx, cy) ----

void drawSun(int cx, int cy, int r, uint16_t color) {
  for (int a = 0; a < 360; a += 45) {
    const float rad = a * 0.01745329252f;
    const int x1 = cx + static_cast<int>(std::cos(rad) * (r + scaled(4)));
    const int y1 = cy + static_cast<int>(std::sin(rad) * (r + scaled(4)));
    const int x2 = cx + static_cast<int>(std::cos(rad) * (r + scaled(11)));
    const int y2 = cy + static_cast<int>(std::sin(rad) * (r + scaled(11)));
    tft.drawWideLine(x1, y1, x2, y2, scaledf(1.5f), color);
  }
  tft.fillCircle(cx, cy, r, color);
}

void drawCloud(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx - scaled(16), cy + scaled(2), scaled(12), color);
  tft.fillCircle(cx + scaled(16), cy + scaled(2), scaled(13), color);
  tft.fillCircle(cx - scaled(3), cy - scaled(9), scaled(15), color);
  tft.fillCircle(cx + scaled(9), cy - scaled(3), scaled(13), color);
  tft.fillRect(cx - scaled(27), cy + scaled(2), scaled(54), scaled(13), color);
}

void drawRaindrops(int cx, int cy, uint16_t color) {
  for (int i = -1; i <= 1; ++i) {
    const int x = cx + i * scaled(14);
    tft.drawWideLine(x, cy, x - scaled(4), cy + scaled(12), scaledf(1.5f), color);
  }
}

void drawSnowflakes(int cx, int cy, uint16_t color) {
  for (int i = -1; i <= 1; ++i) {
    tft.fillCircle(cx + i * scaled(14), cy + scaled(6), scaled(3), color);
  }
}

void drawBolt(int cx, int cy, uint16_t color) {
  tft.fillTriangle(cx - scaled(5), cy, cx + scaled(6), cy, cx - scaled(2),
                   cy + scaled(14), color);
  tft.fillTriangle(cx + scaled(3), cy + scaled(8), cx + scaled(11),
                   cy + scaled(8), cx - scaled(3), cy + scaled(26), color);
}

void drawIcon(Condition cond, int cx, int cy) {
  const uint16_t sun = rgb(255, 200, 0);
  const uint16_t cloud = rgb(205, 210, 220);
  const uint16_t cloud_dark = rgb(120, 132, 150);
  const uint16_t rain = rgb(70, 150, 255);
  const uint16_t snow = rgb(240, 245, 255);
  const uint16_t bolt = rgb(255, 215, 0);
  const uint16_t fog = rgb(180, 190, 200);

  switch (cond) {
    case Condition::Clear:
      drawSun(cx, cy, scaled(20), sun);
      break;
    case Condition::PartlyCloudy:
      drawSun(cx + scaled(12), cy - scaled(12), scaled(12), sun);
      drawCloud(cx - scaled(2), cy + scaled(6), cloud);
      break;
    case Condition::Cloudy:
      drawCloud(cx, cy + scaled(2), cloud);
      break;
    case Condition::Fog:
      drawCloud(cx, cy - scaled(6), fog);
      for (int i = 0; i < 3; ++i) {
        const int y = cy + scaled(16) + i * scaled(7);
        tft.drawWideLine(cx - scaled(22), y, cx + scaled(22), y, scaledf(1.5f),
                         fog);
      }
      break;
    case Condition::Rain:
      drawCloud(cx, cy - scaled(6), cloud_dark);
      drawRaindrops(cx, cy + scaled(14), rain);
      break;
    case Condition::Snow:
      drawCloud(cx, cy - scaled(6), cloud);
      drawSnowflakes(cx, cy + scaled(12), snow);
      break;
    case Condition::Storm:
      drawCloud(cx, cy - scaled(6), cloud_dark);
      drawBolt(cx, cy + scaled(12), bolt);
      break;
    case Condition::Unknown:
    default:
      tft.drawCircle(cx, cy, scaled(18), fog);
      break;
  }
}

/** Draw "[prefix ]NN°F" centered on (cx, y). Degree is a small ring because the
 *  bundled fonts are ASCII-only (no ° glyph). */
void drawTempLine(int cx, int y, float value_f, const lgfx::GFXfont* font,
                  uint16_t color, const char* prefix) {
  displayFontSetBitmap(tft, font);
  tft.setTextColor(color, bgColor());

  char num[8];
  std::snprintf(num, sizeof(num), "%d",
                static_cast<int>(std::lround(value_f)));

  const int h = tft.fontHeight();
  const int ring_r = std::max(scaled(2), h / 9);
  const int kGapNumDeg = scaled(2);
  const int kGapDegF = scaled(1);
  const int kGapPrefix = scaled(6);

  const bool has_prefix = (prefix != nullptr && prefix[0] != '\0');
  const int prefix_w = has_prefix ? tft.textWidth(prefix) + kGapPrefix : 0;
  const int num_w = tft.textWidth(num);
  const int f_w = tft.textWidth("F");
  const int deg_w = kGapNumDeg + ring_r * 2 + kGapDegF;
  const int total = prefix_w + num_w + deg_w + f_w;

  tft.setTextDatum(textdatum_t::top_left);
  const int top = y - h / 2;
  int x = cx - total / 2;

  if (has_prefix) {
    tft.drawString(prefix, x, top);
    x += prefix_w;
  }
  tft.drawString(num, x, top);
  x += num_w + kGapNumDeg;

  const int ring_cx = x + ring_r;
  const int ring_cy = top + ring_r + scaled(1);
  tft.drawCircle(ring_cx, ring_cy, ring_r, color);
  if (ring_r >= scaled(3)) {
    tft.drawCircle(ring_cx, ring_cy, ring_r - 1, color);  // thicker stroke
  }
  x += ring_r * 2 + kGapDegF;

  tft.drawString("F", x, top);
}

}  // namespace

void weatherDisplayDraw() {
  displayFontEnsureLoaded(tft);
  const uint16_t bg = bgColor();
  tft.fillScreen(bg);

  const services::weather::Data& w = services::weather::current();

  if (!w.valid) {
    displayFontSetBitmap(tft, &lgfx_fonts::FreeSansBold12pt7b);
    tft.setTextColor(textColor(), bg);
    tft.setTextDatum(textdatum_t::middle_center);
    tft.drawString("Weather", kCx, scaled(100));
    displayFontSetBitmap(tft, &lgfx_fonts::FreeSans9pt7b);
    tft.drawString(WiFi.status() == WL_CONNECTED ? "Loading..." : "No Wi-Fi",
                   kCx, scaled(134));
    tft.setTextDatum(textdatum_t::top_left);
    return;
  }

  drawIcon(w.condition, kCx, scaled(70));

  displayFontSetBitmap(tft, &lgfx_fonts::FreeSansBold12pt7b);
  tft.setTextColor(textColor(), bg);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.drawString(w.label, kCx, scaled(126));

  drawTempLine(kCx, scaled(162), w.temp_f, &lgfx_fonts::FreeSansBold18pt7b,
               textColor(), nullptr);
  drawTempLine(kCx, scaled(198), w.feels_f, &lgfx_fonts::FreeSans9pt7b,
               rgb(180, 190, 205), "Feels");

  tft.setTextDatum(textdatum_t::top_left);
}

}  // namespace ui
