#pragma once

/** Picks the LovyanGFX device matching the board selected in config.h. */

#include "config.h"

#if defined(BOARD_DISPLAY_ST7701_RGB)
#include "hardware/lgfx_st7701.hpp"
#else
#include "hardware/lgfx_gc9a01.hpp"
#endif
