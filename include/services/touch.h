#pragma once

namespace services::touch {

/** Gestures recognised by the sampler. One is reported per contact session. */
enum class Gesture {
  None,
  Tap,
  SwipeLeft,
  SwipeRight,
  SwipeUp,
  SwipeDown,
  PinchIn,   // fingers moved together -> zoom out
  PinchOut,  // fingers moved apart -> zoom in
};

/**
 * Reset the touch controller and detect which one is fitted (GT911 or the
 * CST816 family). Starts the shared I2C bus. Call once in setup().
 */
void init();

/**
 * One sampling step. Driven by the input task in main.cpp every ~20 ms, so
 * touches are still caught while the loop is busy fetching ADS-B data or
 * repainting the radar. Safe to call from any task.
 */
void update();

/** Pop the gesture detected since the last call (Gesture::None if idle). */
Gesture consume();

/** Compatibility helper: true exactly once per tap. */
bool tapped();

/** Log line describing the detected controller (for diagnostics). */
const char* controllerName();

}  // namespace services::touch
