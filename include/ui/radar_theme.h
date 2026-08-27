#pragma once

#include <cstdint>

#include "ui/ui_scale.h"

namespace ui::radar {

/** Layout follows the panel: 240×240 on the 1.28" board, 480×480 on the 2.8C. */
constexpr int kSize = config::kDisplayWidth;
constexpr int kCenterX = kSize / 2;
constexpr int kCenterY = kSize / 2;

/** Outermost grid ring (inside edge labels). */
constexpr int kGridOuterRadius = scaled(107);

/** N: offset from top edge (top_center, negative = up). */
constexpr int kCardinalNorthOffsetY = scaled(-1);
/** S: offset from bottom edge (bottom_center, positive = down). */
constexpr int kCardinalSouthOffsetY = scaled(3);

/** Gap between scale label right edge and outer ring on the east spoke (px). */
constexpr int kScaleGapFromOuterRing = scaled(6);

/** Target cap height (px) for N/S/E/W. */
constexpr int kCardinalLabelHeightPx = scaled(14);
/** Scale label is this many px shorter than cardinals. */
constexpr int kScaleBelowCardinalPx = scaled(3);

constexpr int kRingCount = 4;

/** Shared grid stroke: drawWideLine half-width (~2 px total); rings use the same px count. */
constexpr float kGridStrokeHalfWidth = scaledf(1.0f);

constexpr int kCenterDotRadius = scaled(2);

/** Filled aircraft symbol (nose triangle). */
constexpr int kAircraftNoseLenPx = scaled(8);
constexpr int kAircraftTailLenPx = scaled(3);
constexpr int kAircraftTailHalfPx = scaled(4);
/** Track vector: ground distance covered in this many seconds at current gs. */
constexpr float kAircraftTrackHorizonSec = 60.0f;
/** Minimum visible vector when gs > 0 (px). */
constexpr int kAircraftSpeedLineMinPx = scaled(2);
/** Track line length uses this outer_km, not the active range preset. */
constexpr float kAircraftTrackRefOuterKm = 13.3f;
/** Shorter than full 60 s horizon at ref scale; ×1.5 length boost applied. */
constexpr float kAircraftTrackLengthScale = 1.5f / 5.0f;
/** drawWideLine half-width for speed vectors (~2 px total). */
constexpr float kAircraftTrackLineHalfWidth = scaledf(1.0f);

constexpr float kRunwayLineWidthPx = scaledf(2.0f);
constexpr float kRunwayLineHalfWidth = kRunwayLineWidthPx * 0.5f;
constexpr int kRunwayLabelHeightPx = kCardinalLabelHeightPx;
constexpr int kRunwayLabelGapPx = scaled(3);
/** Gap from triangle edge to tag block (px). */
constexpr int kAircraftLabelGapPx = scaled(1);
/** Keep symbol centroid inside outer ring by at least this inset (px). */
constexpr int kAircraftInsideRingInsetPx =
    kAircraftNoseLenPx + kAircraftTailHalfPx + scaled(1);

/** Beyond-ring traffic: bearing cues on screen rim (correct direction, fixed radius). */
constexpr int kBeyondRingDotRadiusPx = scaled(4);
constexpr int kBeyondRingScreenMarginPx = scaled(2);
/** Target cap height (px) for aircraft tags (bold, slightly above scale label). */
constexpr int kAircraftTagLabelHeightPx = scaled(13);

/** RGB565 palette targets (applied in initPalette). */
constexpr uint8_t kBgR = 4;
constexpr uint8_t kBgG = 10;
constexpr uint8_t kBgB = 28;
constexpr uint8_t kGridR = 16;
constexpr uint8_t kGridG = 100;
constexpr uint8_t kGridB = 32;
constexpr uint8_t kAircraftR = 255;
constexpr uint8_t kAircraftG = 0;
constexpr uint8_t kAircraftB = 0;
constexpr uint8_t kTrackR = 255;
constexpr uint8_t kTrackG = 0;
constexpr uint8_t kTrackB = 255;
constexpr uint8_t kTagTypeR = 255;
constexpr uint8_t kTagTypeG = 200;
constexpr uint8_t kTagTypeB = 0;
constexpr uint8_t kTagAltR = 90;
constexpr uint8_t kTagAltG = 200;
constexpr uint8_t kTagAltB = 255;
constexpr uint8_t kRunwayR = 56;
constexpr uint8_t kRunwayG = 150;
constexpr uint8_t kRunwayB = 170;
/** Lighter teal for ICAO labels (vs runway lines). */
constexpr uint8_t kRunwayLabelR = 110;
constexpr uint8_t kRunwayLabelG = 210;
constexpr uint8_t kRunwayLabelB = 230;

extern uint16_t kColorBackground;
extern uint16_t kColorGrid;
extern uint16_t kColorLabel;
extern uint16_t kColorCenter;
extern uint16_t kColorAircraft;
extern uint16_t kColorTrackVector;
extern uint16_t kColorTagType;
extern uint16_t kColorTagAltitude;
extern uint16_t kColorRunway;
extern uint16_t kColorRunwayLabel;

}  // namespace ui::radar
