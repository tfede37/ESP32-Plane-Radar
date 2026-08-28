#include "services/touch.h"

#include <Arduino.h>
#include <Wire.h>

#include <cmath>

#include "config.h"
#include "hardware/io_expander.h"
#include "ui/ui_scale.h"

namespace services::touch {

namespace {

// ---------------------------------------------------------------------------
// Controller drivers
//
// The panel fitted to a board is detected at boot instead of being hard-coded:
// Waveshare has shipped the same model with different touch ICs, and a wrong
// guess would leave the screen silently unresponsive.
// ---------------------------------------------------------------------------

enum class Controller { None, GT911, CST816 };

constexpr uint8_t kGt911Addr1 = 0x5D;
constexpr uint8_t kGt911Addr2 = 0x14;
constexpr uint8_t kCst816Addr = 0x15;

constexpr int kMaxPoints = 2;  // all we need: tap, swipe, pinch

struct Point {
  int x = 0;
  int y = 0;
};

Controller s_controller = Controller::None;
uint8_t s_addr = 0;
const char* s_controller_name = "none";
uint32_t s_read_errors = 0;

bool i2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

// ---- GT911: 16-bit register addresses ----
constexpr uint16_t kGtRegStatus = 0x814E;
constexpr uint16_t kGtRegPoints = 0x8150;
constexpr uint16_t kGtRegProductId = 0x8140;

bool gtRead(uint16_t reg, uint8_t* buf, size_t len) {
  Wire.beginTransmission(s_addr);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  const size_t got =
      Wire.requestFrom(static_cast<int>(s_addr), static_cast<int>(len));
  if (got != len) {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buf[i] = Wire.read();
  }
  return true;
}

bool gtWrite(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(s_addr);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool gtReadPoints(Point* pts, int* count, bool* fresh, bool* coords_valid) {
  *fresh = false;
  *coords_valid = false;
  uint8_t status = 0;
  if (!gtRead(kGtRegStatus, &status, 1)) {
    return false;
  }
  if ((status & 0x80) == 0) {
    return true;  // no new report; keep the previous state
  }

  int n = status & 0x0F;
  if (n > 5) {
    n = 0;  // corrupt report
  }
  const int wanted = n > kMaxPoints ? kMaxPoints : n;
  uint8_t buf[kMaxPoints * 8] = {0};
  // A failed coordinate read still tells us a finger is down: report the
  // contact without coordinates rather than dropping the whole touch.
  if (wanted > 0 && gtRead(kGtRegPoints, buf, wanted * 8)) {
    for (int i = 0; i < wanted; ++i) {
      const uint8_t* p = buf + i * 8;
      pts[i].x = static_cast<int>(p[1]) | (static_cast<int>(p[2]) << 8);
      pts[i].y = static_cast<int>(p[3]) | (static_cast<int>(p[4]) << 8);
    }
    *coords_valid = true;
  }
  *count = wanted;
  *fresh = true;
  gtWrite(kGtRegStatus, 0x00);  // ack, arms the next report
  return true;
}

// ---- CST816 family: 8-bit register addresses, single touch ----
bool cstRead(uint8_t reg, uint8_t* buf, size_t len) {
  Wire.beginTransmission(s_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  const size_t got =
      Wire.requestFrom(static_cast<int>(s_addr), static_cast<int>(len));
  if (got != len) {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buf[i] = Wire.read();
  }
  return true;
}

bool cstReadPoints(Point* pts, int* count, bool* fresh, bool* coords_valid) {
  uint8_t buf[5] = {0};  // 0x02 fingers, 0x03..0x06 X/Y
  if (!cstRead(0x02, buf, sizeof(buf))) {
    *fresh = false;
    *coords_valid = false;
    return false;
  }
  const int n = (buf[0] & 0x0F) > 0 ? 1 : 0;
  if (n > 0) {
    pts[0].x = ((buf[1] & 0x0F) << 8) | buf[2];
    pts[0].y = ((buf[3] & 0x0F) << 8) | buf[4];
  }
  *count = n;
  *fresh = true;
  *coords_valid = true;
  return true;
}

bool readPoints(Point* pts, int* count, bool* fresh, bool* coords_valid) {
  switch (s_controller) {
    case Controller::GT911:
      return gtReadPoints(pts, count, fresh, coords_valid);
    case Controller::CST816:
      return cstReadPoints(pts, count, fresh, coords_valid);
    default:
      *fresh = false;
      *coords_valid = false;
      return false;
  }
}

// ---------------------------------------------------------------------------
// Reset + detection
// ---------------------------------------------------------------------------

void resetController() {
#if defined(BOARD_HAS_IO_EXPANDER)
  // 2.8C: touch reset hangs off EXIO2. Holding INT low across the rising edge
  // of reset selects GT911 address 0x5D.
  hardware::expander::begin();
  pinMode(static_cast<int>(config::kTouchPinInt), OUTPUT);
  digitalWrite(static_cast<int>(config::kTouchPinInt), LOW);
  hardware::expander::write(config::kExioTouchReset, false);
  delay(20);
  hardware::expander::write(config::kExioTouchReset, true);
  delay(100);
  pinMode(static_cast<int>(config::kTouchPinInt), INPUT);
  delay(50);
#else
  // 1.28": dedicated reset GPIO; CST816 needs ~50 ms to boot afterwards.
  pinMode(static_cast<uint8_t>(config::kTouchPinRst), OUTPUT);
  digitalWrite(static_cast<uint8_t>(config::kTouchPinRst), LOW);
  delay(10);
  digitalWrite(static_cast<uint8_t>(config::kTouchPinRst), HIGH);
  delay(60);
  pinMode(static_cast<int>(config::kTouchPinInt), INPUT_PULLUP);
#endif
}

void logBusScan() {
  Serial.print("touch: I2C devices:");
  bool any = false;
  for (uint8_t addr = 0x08; addr < 0x78; ++addr) {
    if (i2cProbe(addr)) {
      Serial.printf(" 0x%02X", addr);
      any = true;
    }
  }
  Serial.println(any ? "" : " none");
}

void detectController() {
  for (const uint8_t addr : {kGt911Addr1, kGt911Addr2}) {
    if (!i2cProbe(addr)) {
      continue;
    }
    s_addr = addr;
    s_controller = Controller::GT911;
    uint8_t id[4] = {0};
    if (gtRead(kGtRegProductId, id, sizeof(id))) {
      static char name[24];
      snprintf(name, sizeof(name), "GT%c%c%c%c @0x%02X", id[0] ? id[0] : '?',
               id[1] ? id[1] : '?', id[2] ? id[2] : '?', id[3] ? id[3] : '?',
               addr);
      s_controller_name = name;
      gtWrite(kGtRegStatus, 0x00);
      return;
    }
  }

  if (i2cProbe(kCst816Addr)) {
    s_addr = kCst816Addr;
    s_controller = Controller::CST816;
    uint8_t chip = 0;
    cstRead(0xA7, &chip, 1);
    static char name[24];
    snprintf(name, sizeof(name), "CST816 (chip 0x%02X)", chip);
    s_controller_name = name;
    // Disable auto-sleep so the controller keeps ACKing when idle (0xFE = 1).
    Wire.beginTransmission(s_addr);
    Wire.write(0xFE);
    Wire.write(0x01);
    Wire.endTransmission();
    return;
  }

  s_controller = Controller::None;
  s_controller_name = "not found";
}

// ---------------------------------------------------------------------------
// Gesture state machine
// ---------------------------------------------------------------------------

// Thresholds in 240 px design units, scaled to the panel.
const int kSwipeMinTravelPx = ui::scaled(45);
constexpr unsigned long kContactMinMs = 30;
constexpr float kPinchMinRatio = 1.25f;

bool s_ready = false;
bool s_tracking = false;
unsigned long s_start_ms = 0;
Point s_start{};
Point s_last{};
int s_max_fingers = 0;
bool s_have_coords = false;
float s_pinch_start = 0.0f;
float s_pinch_last = 0.0f;
float s_pinch_min = 0.0f;
float s_pinch_max = 0.0f;

portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
volatile Gesture s_pending = Gesture::None;

const char* gestureName(Gesture g) {
  switch (g) {
    case Gesture::Tap: return "tap";
    case Gesture::SwipeLeft: return "swipe-left";
    case Gesture::SwipeRight: return "swipe-right";
    case Gesture::SwipeUp: return "swipe-up";
    case Gesture::SwipeDown: return "swipe-down";
    case Gesture::PinchIn: return "pinch-in";
    case Gesture::PinchOut: return "pinch-out";
    default: return "none";
  }
}

void publish(Gesture g) {
  portENTER_CRITICAL(&s_mux);
  s_pending = g;
  portEXIT_CRITICAL(&s_mux);
}

float distance(const Point& a, const Point& b) {
  const float dx = static_cast<float>(a.x - b.x);
  const float dy = static_cast<float>(a.y - b.y);
  return sqrtf(dx * dx + dy * dy);
}

Gesture classifyRelease(unsigned long now) {
  const unsigned long held = now - s_start_ms;
  if (held < kContactMinMs) {
    return Gesture::None;  // contact bounce
  }
  if (!s_have_coords) {
    // Controller acknowledged a contact but no coordinates came through: the
    // safe reading is a plain tap.
    return Gesture::Tap;
  }

  if (s_max_fingers >= 2 && s_pinch_start > 1.0f) {
    const float grow = s_pinch_max / s_pinch_start;
    const float shrink = s_pinch_start / (s_pinch_min > 1.0f ? s_pinch_min : 1.0f);
    if (grow >= kPinchMinRatio || shrink >= kPinchMinRatio) {
      return (s_pinch_last >= s_pinch_start) ? Gesture::PinchOut : Gesture::PinchIn;
    }
  }

  const int dx = s_last.x - s_start.x;
  const int dy = s_last.y - s_start.y;
  const int adx = abs(dx);
  const int ady = abs(dy);

  if (adx >= kSwipeMinTravelPx || ady >= kSwipeMinTravelPx) {
    if (adx >= ady) {
      return dx > 0 ? Gesture::SwipeRight : Gesture::SwipeLeft;
    }
    return dy > 0 ? Gesture::SwipeDown : Gesture::SwipeUp;
  }

  // Anything else that stayed put is a tap, however long it was held: there is
  // no long-press action on screen, and silently dropping a slow press just
  // looks like a dead touchscreen.
  return Gesture::Tap;
}

}  // namespace

void init() {
  hardware::i2cBegin();
  resetController();
  detectController();

  Serial.printf("touch: controller %s\n", s_controller_name);
  if (s_controller == Controller::None) {
    logBusScan();
  }
  s_ready = s_controller != Controller::None;
}

void update() {
  if (!s_ready) {
    return;
  }

  Point pts[kMaxPoints]{};
  int count = 0;
  bool fresh = false;
  bool coords_valid = false;
  if (!readPoints(pts, &count, &fresh, &coords_valid)) {
    if ((++s_read_errors % 100) == 1) {
      Serial.printf("touch: I2C read failed (%u)\n",
                    static_cast<unsigned>(s_read_errors));
    }
    return;
  }
  if (!fresh) {
    return;
  }

  const unsigned long now = millis();

  if (count > 0) {
    if (!s_tracking) {
      s_tracking = true;
      s_start_ms = now;
      s_start = pts[0];
      s_last = pts[0];
      s_max_fingers = 0;
      s_have_coords = false;
      s_pinch_start = 0.0f;
      s_pinch_min = 0.0f;
      s_pinch_max = 0.0f;
    }
    if (coords_valid) {
      if (!s_have_coords) {
        s_start = pts[0];  // first trustworthy position of this contact
        s_have_coords = true;
      }
      s_last = pts[0];
    }
    if (count > s_max_fingers) {
      s_max_fingers = count;
    }
    if (coords_valid && count >= 2) {
      const float d = distance(pts[0], pts[1]);
      if (s_pinch_start <= 0.0f) {
        s_pinch_start = d;
        s_pinch_min = d;
        s_pinch_max = d;
      }
      s_pinch_last = d;
      if (d < s_pinch_min) {
        s_pinch_min = d;
      }
      if (d > s_pinch_max) {
        s_pinch_max = d;
      }
    }
    return;
  }

  if (!s_tracking) {
    return;
  }
  s_tracking = false;

  const Gesture g = classifyRelease(now);
  if (g == Gesture::None) {
    return;
  }
  Serial.printf("touch: %s (%d,%d -> %d,%d, %d finger%s)\n", gestureName(g),
                s_start.x, s_start.y, s_last.x, s_last.y, s_max_fingers,
                s_max_fingers == 1 ? "" : "s");
  publish(g);
}

Gesture consume() {
  portENTER_CRITICAL(&s_mux);
  const Gesture g = s_pending;
  s_pending = Gesture::None;
  portEXIT_CRITICAL(&s_mux);
  return g;
}

bool tapped() { return consume() == Gesture::Tap; }

const char* controllerName() { return s_controller_name; }

}  // namespace services::touch
