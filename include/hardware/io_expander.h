#pragma once

#include <cstdint>

#include "config.h"

namespace hardware {

/** Start the shared I2C bus (touch, I/O expander, …). Safe to call twice. */
void i2cBegin();

#if defined(BOARD_HAS_IO_EXPANDER)
/**
 * TCA9554PWR I/O expander (8 outputs, EXIO1..EXIO8 = bit 0..7).
 * On the 2.8C board it carries LCD reset/CS and the touch reset line.
 */
namespace expander {

/** Configure every pin as output (all HIGH except the buzzer). Idempotent. */
bool begin();
/** Drive one expander pin; returns false if the I2C write failed. */
bool write(uint8_t pin, bool level);

}  // namespace expander
#endif

}  // namespace hardware
