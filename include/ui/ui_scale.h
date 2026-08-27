#pragma once

#include "config.h"

namespace ui {

/**
 * The UI was laid out for the 240×240 round panel. Every hard-coded pixel value
 * goes through scaled()/scaledf() so a larger panel (e.g. the 480×480 2.8C)
 * gets the same design, just bigger.
 */
constexpr int kLayoutBaseSize = 240;

constexpr int scaled(int px) { return px * config::kDisplayWidth / kLayoutBaseSize; }

constexpr float kUiScale =
    static_cast<float>(config::kDisplayWidth) / static_cast<float>(kLayoutBaseSize);

constexpr float scaledf(float px) { return px * kUiScale; }

/** VLW size the layout was authored against (px). */
constexpr float kLayoutFontPx = 15.0f;

/**
 * Multiplier applied to smooth-font sizes. Bigger panels embed a bigger VLW cut
 * instead of upscaling the small one, so this stays ~1.0 and text stays crisp.
 */
constexpr float kSmoothFontScale = kUiScale * kLayoutFontPx / config::kEmbeddedVlwFontPx;

/** Integer multiplier for bitmap fonts (1 on 240 px panels, 2 on 480 px). */
constexpr int kTextScale =
    config::kDisplayWidth / kLayoutBaseSize > 0 ? config::kDisplayWidth / kLayoutBaseSize : 1;

}  // namespace ui
