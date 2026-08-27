#include "hardware/io_expander.h"

#include <Arduino.h>
#include <Wire.h>

namespace hardware {

namespace {

bool s_i2c_ready = false;

}  // namespace

void i2cBegin() {
  if (s_i2c_ready) {
    return;
  }
  Wire.begin(static_cast<int>(config::kTouchPinSda),
             static_cast<int>(config::kTouchPinScl),
             config::kI2cFrequencyHz);
  s_i2c_ready = true;
}

#if defined(BOARD_HAS_IO_EXPANDER)

namespace expander {

namespace {

// TCA9554 registers.
constexpr uint8_t kRegOutput = 0x01;
constexpr uint8_t kRegConfig = 0x03;  // 1 = input, 0 = output

bool s_ready = false;
/** Shadow of the output register (the chip is write-mostly here). */
uint8_t s_output = 0xFF;

bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(config::kIoExpanderI2cAddr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

}  // namespace

bool begin() {
  if (s_ready) {
    return true;
  }
  i2cBegin();

  // Idle state: everything released (HIGH) with the buzzer off (LOW).
  s_output = static_cast<uint8_t>(0xFF & ~(1u << config::kExioBuzzer));
  const bool ok = writeReg(kRegOutput, s_output) && writeReg(kRegConfig, 0x00);
  if (!ok) {
    Serial.println("expander: TCA9554 not responding");
    return false;
  }
  s_ready = true;
  return true;
}

bool write(uint8_t pin, bool level) {
  if (pin > 7) {
    return false;
  }
  const uint8_t mask = static_cast<uint8_t>(1u << pin);
  const uint8_t next =
      level ? static_cast<uint8_t>(s_output | mask)
            : static_cast<uint8_t>(s_output & ~mask);
  if (!writeReg(kRegOutput, next)) {
    return false;
  }
  s_output = next;
  return true;
}

}  // namespace expander

#endif  // BOARD_HAS_IO_EXPANDER

}  // namespace hardware
