#include "services/touch.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "hardware/io_expander.h"

namespace services::touch {

namespace {

bool s_ready = false;
bool s_in_touch = false;           // finger currently down (level state)
unsigned long s_last_tap_ms = 0;   // for the blocked-window fallback guard

constexpr int kIntPin = static_cast<int>(config::kTouchPinInt);
constexpr unsigned long kQuietGapMs = 300;

// Set by the INT-pin interrupt. Used only as a fallback: it catches a tap that
// starts AND ends while the main loop is blocked in an HTTP fetch, where the
// I2C polling below would otherwise never see the finger.
volatile bool s_touch_irq = false;

void IRAM_ATTR onTouchIsr() { s_touch_irq = true; }

#if defined(BOARD_TOUCH_GT911)

// ---- GT911 (Waveshare 2.8C): 16-bit register addresses -------------------
constexpr uint16_t kRegStatus = 0x814E;
constexpr uint16_t kRegProductId = 0x8140;

bool readReg16(uint16_t reg, uint8_t* buf, size_t len) {
  Wire.beginTransmission(config::kTouchI2cAddr);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  const size_t got = Wire.requestFrom(static_cast<int>(config::kTouchI2cAddr),
                                      static_cast<int>(len));
  if (got != len) {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buf[i] = Wire.read();
  }
  return true;
}

bool writeReg16(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(config::kTouchI2cAddr);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

/** Reset pulse with INT held low, which selects I2C address 0x5D. */
void controllerReset() {
  hardware::expander::begin();

  pinMode(kIntPin, OUTPUT);
  digitalWrite(kIntPin, LOW);
  hardware::expander::write(config::kExioTouchReset, false);
  delay(20);
  hardware::expander::write(config::kExioTouchReset, true);
  delay(100);
  pinMode(kIntPin, INPUT);
  delay(50);
}

void controllerInit() {
  controllerReset();

  uint8_t id[4] = {0};
  if (readReg16(kRegProductId, id, sizeof(id))) {
    Serial.printf("touch: GT911 id %c%c%c%c\n", id[0], id[1], id[2], id[3]);
  } else {
    Serial.println("touch: GT911 not responding");
  }
  writeReg16(kRegStatus, 0x00);
}

/**
 * Poll the coordinate status register. Returns false when the controller has
 * no fresh report, in which case *down keeps its previous value.
 */
bool pollDown(bool* down) {
  uint8_t status = 0;
  if (!readReg16(kRegStatus, &status, 1)) {
    return false;
  }
  if ((status & 0x80) == 0) {  // buffer not ready -> nothing new
    return false;
  }
  *down = (status & 0x0F) > 0;
  writeReg16(kRegStatus, 0x00);  // ack the report
  return true;
}

#else

// ---- CST816 (Waveshare 1.28): 8-bit register addresses -------------------
bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(config::kTouchI2cAddr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readReg(uint8_t reg, uint8_t* buf, size_t len) {
  Wire.beginTransmission(config::kTouchI2cAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  const size_t got = Wire.requestFrom(
      static_cast<int>(config::kTouchI2cAddr), static_cast<int>(len));
  if (got != len) {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buf[i] = Wire.read();
  }
  return true;
}

void controllerInit() {
  // Hardware reset pulse (CST816 needs ~50 ms to boot afterwards).
  pinMode(static_cast<uint8_t>(config::kTouchPinRst), OUTPUT);
  digitalWrite(static_cast<uint8_t>(config::kTouchPinRst), LOW);
  delay(10);
  digitalWrite(static_cast<uint8_t>(config::kTouchPinRst), HIGH);
  delay(60);

  // Disable auto-sleep so the controller keeps ACKing the bus when idle, so the
  // level polling below works without I2C errors (0xFE = 1).
  writeReg(0xFE, 0x01);
}

/** Level state: number of fingers currently touching (reg 0x02 = 0 or 1). */
bool pollDown(bool* down) {
  uint8_t fingers = 0;
  if (!readReg(0x02, &fingers, 1)) {
    *down = false;
    return false;
  }
  *down = fingers > 0;
  return true;
}

#endif  // BOARD_TOUCH_GT911

}  // namespace

void init() {
  hardware::i2cBegin();
  controllerInit();

  pinMode(kIntPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kIntPin), onTouchIsr, FALLING);

  s_ready = true;
}

bool tapped() {
  if (!s_ready) {
    return false;
  }

  bool down = s_in_touch;
  pollDown(&down);

  const bool irq = s_touch_irq;
  s_touch_irq = false;
  const unsigned long now = millis();

  bool tap = false;
  if (down) {
    if (!s_in_touch) {  // rising edge of a press -> one tap
      s_in_touch = true;
      tap = true;
    }
  } else {
    if (s_in_touch) {
      s_in_touch = false;  // finger lifted; session over
    } else if (irq && now - s_last_tap_ms >= kQuietGapMs) {
      // A whole tap happened between two polls (loop was blocked); the INT
      // interrupt caught it even though the finger is already up.
      tap = true;
    }
  }

  if (tap) {
    s_last_tap_ms = now;
  }
  return tap;
}

}  // namespace services::touch
