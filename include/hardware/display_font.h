#pragma once

#include <LovyanGFX.hpp>

bool displayFontInit();
bool displayFontIsSmooth();

/** Load embedded VLW font on gfx if smooth fonts are enabled and not already active. */
bool displayFontEnsureLoaded(lgfx::LGFXBase& gfx);

/** VLW: setTextSize scale (1.0 = font point size at 240 px; scaled per panel). */
void displayFontSetSmoothSize(lgfx::LGFXBase& gfx, float size);

/** Bitmap GFXfont fallback (scaled per panel); clears any runtime VLW font. */
void displayFontSetBitmap(lgfx::LGFXBase& gfx, const lgfx::GFXfont* font);
