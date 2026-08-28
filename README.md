# Plane Radar + Weather (ESP32-S3 Touch)

Firmware for Waveshare's round ESP32-S3 touch displays. Shows a circular **ADS-B radar** around your location, plus a **weather page** you reach by tapping the screen. **WiFiManager** handles first-time setup.

| Board | Panel | Touch | PlatformIO env |
|-------|-------|-------|----------------|
| [**ESP32-S3-Touch-LCD-1.28**](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28) | 1.28″ round GC9A01, 240×240, SPI | CST816 | `supermini` (default) |
| [**ESP32-S3-Touch-LCD-2.8C**](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.8C) | 2.8″ round ST7701, 480×480, 16-bit RGB | GT911 | `waveshare-28c` |

The whole UI is resolution-aware: every coordinate is authored for 240×240 and scaled to the active panel (`include/ui/ui_scale.h`), and the 480×480 build embeds a 30 px smooth font instead of upscaling the 15 px one.

> **Fork note:** this is an ESP32-S3 port of [**ESP32-Plane-Radar** by MatixYo](https://github.com/MatixYo/ESP32-Plane-Radar) (MIT). The original targets an ESP32-C3 Super Mini; this fork retargets the Waveshare S3 touch board and adds a weather page and tap-to-switch. All original radar functionality is preserved.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadar-Setup`**
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid
3. **Weather** — current conditions from the [US National Weather Service](https://www.weather.gov/documentation/services-web-api) (matches Apple Weather / NWS), with [Open-Meteo](https://open-meteo.com) as a worldwide fallback — both free, no API key

**Tap anywhere on the screen to switch between the radar and the weather page; tap again to switch back.**

After Wi‑Fi is saved, the device reconnects automatically; the radar runs in the main loop with periodic ADS-B updates (~3 s), and the weather refreshes every 10 minutes while shown.

## Flash it (easiest — no build tools)

Flash straight from your browser with the one-click web installer (Chrome or Edge on desktop):

### 👉 [Open the Web Installer](https://turbotime29.github.io/ESP32-Plane-Radar/)

> The web installer serves the **1.28″** image. For the **2.8C**, download `plane-radar-28c-vX.Y.Z.bin` from [Releases](../../releases/latest) and flash it at offset `0x0` (see below), or build `waveshare-28c` from source.

1. Plug the board into your computer with USB-C.
2. Open the link above, click **Connect**, and pick the serial port. (If it isn't detected, put it in download mode: **hold BOOT, tap RESET, release BOOT**, then retry.)
3. Click **Install** and wait about a minute.
4. On first boot, join the **`PlaneRadar-Setup`** Wi‑Fi and open `http://192.168.4.1` to enter your home Wi‑Fi and latitude/longitude.

<details>
<summary>Prefer to flash manually?</summary>

Grab the latest `plane-radar-vX.Y.Z.bin` from **[Releases](../../releases/latest)**, open the
**[ESP Web Flasher](https://espressif.github.io/esptool-js/)**, connect the board, and add the
`.bin` at offset **`0x0`** with the chip set to **ESP32-S3**, then **Program**. It's a merged image
(bootloader + partitions + app), so the single file at `0x0` is all you need; a `.sha256` is provided
to verify the download.
</details>

To build it yourself instead, see [Build from source](#build-from-source).

## Controls

| Input | Effect |
|-------|--------|
| **Tap screen** | Toggle between radar and weather page |
| **Pinch out / swipe right / swipe up** (radar) | Zoom in — next shorter range |
| **Pinch in / swipe left / swipe down** (radar) | Zoom out — next longer range |
| **BOOT short tap** | Cycle range preset (5 → 10 → 15 → 25 km); saved to flash |
| **BOOT hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

BOOT is the on-board button on **GPIO 0** (active LOW). During setup you can also hold BOOT at power-on to force a credential reset.

The ADS-B fetch and the input sampling both run on their own tasks, so `loop()` only paints: a touch or a BOOT press is acted on within a frame instead of queueing behind a multi-second HTTP round trip.

At boot the panel shows a splash with the firmware version, the board and the touch controller that answered on the bus, so the hardware can be checked without a serial console. Range changes and BOOT presses are confirmed by a short label on screen.

Touch and BOOT are sampled by a dedicated input task every 20 ms, so gestures register even while the loop is busy with an ADS-B fetch or a full repaint. Pinch needs a multi-touch controller (GT911 on the 2.8C); the 1.28″ CST816 is single-touch, so use swipes there. The touch IC is probed at boot (GT911 at `0x5D`/`0x14`, CST816 family at `0x15`) and every gesture is logged over serial — the quickest way to see what the panel actually reported:

```
touch: controller GT911 @0x5D
touch: swipe-left (392,208 -> 96,214, 1 finger)
Range: 15km (outer ~20 km)
BOOT: tap (118 ms)
```

## Weather page

Uses the US National Weather Service for your configured location (the source Apple Weather tracks), falling back to Open-Meteo where NWS has no coverage. No API key:

- **Condition** label (Sunny / Partly Cloudy / Cloudy / Foggy / Rainy / Snowy / Storm)
- **Color icon** for the condition
- **Temperature** in °F and **real feel** (apparent temperature) in °F

Layout and icons: `src/ui/weather_display.cpp`. Refresh interval: `kWeatherFetchIntervalMs` in `config.h`.

## Wi‑Fi setup portal

1. Connect to **`PlaneRadar-Setup`**
2. Open **`http://plane-radar.local`** (preferred) or **`http://192.168.4.1`** — both are shown on the yellow setup screen; the captive portal may open automatically
3. Set home Wi‑Fi, then save

**Custom fields** (stored in NVS):

| Field | Purpose |
|-------|---------|
| **Latitude / Longitude** | Radar center, ADS-B query position, **and weather location** |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** |

## Radar display

- Dark background, subdued green rings and crosshairs; white **N / S / E / W** at the bezel and a range label on the east spoke
- **Inside the outer ring** — red heading triangle, magenta speed vector, callsign / type / altitude tags
- **Outside the ring** — small red dot on the rim at the correct bearing
- Position math uses a `cos(latitude)` correction so east–west distances are accurate away from the equator

### Range presets

| Ring 3 label | Outer radius (aircraft scale) |
|------------|-------------------------------|
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Preset and miles/km choice persist across reboot (`planeradar` NVS namespace).

## Wiring (Waveshare ESP32-S3-Touch-LCD-1.28)

The display and touch are on-board; no manual wiring is needed. Pins are defined in `include/boards/waveshare_lcd_1_28.h`:

| Function | GPIO |
|----------|------|
| Display SCLK | 10 |
| Display MOSI (SDA) | 11 |
| Display CS | 9 |
| Display DC | 8 |
| Display RST | 14 |
| Backlight (PWM) | 2 |
| Touch I²C SDA | 6 |
| Touch I²C SCL | 7 |
| Touch INT | 5 |
| Touch RST | 13 |
| BOOT button | 0 |

## Wiring (Waveshare ESP32-S3-Touch-LCD-2.8C)

Also fully on-board. Pins live in `include/boards/waveshare_lcd_2_8c.h`; note that the panel reset/CS and the touch reset hang off a **TCA9554 I/O expander** at `0x20` (driven by `src/hardware/io_expander.cpp`):

| Function | GPIO |
|----------|------|
| RGB PCLK / DE / VSYNC / HSYNC | 41 / 40 / 39 / 38 |
| RGB red R1–R5 | 46, 3, 8, 18, 17 |
| RGB green G0–G5 | 14, 13, 12, 11, 10, 9 |
| RGB blue B1–B5 | 5, 45, 48, 47, 21 |
| Panel init SPI SDA / SCL | 1 / 2 |
| Backlight (PWM) | 6 |
| I²C SDA / SCL (touch, expander, IMU, RTC) | 15 / 7 |
| Touch INT | 16 |
| BOOT button | 0 |
| LCD reset / touch reset / LCD CS | EXIO1 / EXIO2 / EXIO3 (expander) |

The ST7701 gets the vendor init sequence from `include/hardware/lgfx_st7701.hpp`; the 450 KB framebuffer lives in the board's octal PSRAM, so the `qio_opi` memory type in `platformio.ini` is required. If red and blue come out swapped on your unit, flip `kRgbSwapRedBlue` in the board header.

## Configuration

Behavior lives in **`include/config.h`**, hardware in **`include/boards/<board>.h`**:

| Area | Keys / notes |
|------|----------------|
| Display bus | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` (1.28) / RGB pins + timings, `kRgbSwapRedBlue` (2.8C) |
| Display offset | `kDisplayOffsetX/Y` — shift output to clear the bezel if needed |
| Backlight | `kDisplayBrightness` (0-255) |
| Touch | `kTouchPinSda/Scl/Int/Rst`; the controller (GT911 / CST816) is probed at boot |
| UI scale | `include/ui/ui_scale.h` — everything is derived from `kDisplayWidth` |
| Weather | `kWeatherFetchIntervalMs`, `kWeatherRetryIntervalMs` |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |

Range presets: `include/ui/radar_range.h` (`kRangePresets`).

## Build from source

Requires [PlatformIO](https://platformio.org/). To build, flash, and watch the serial log:

```bash
# 1.28" board
pio run -e supermini -t upload --upload-port <PORT>       # e.g. COM3 on Windows
# 2.8C board
pio run -e waveshare-28c -t upload --upload-port <PORT>
pio device monitor --port <PORT>
```

- PlatformIO envs: **`supermini`** and **`waveshare-28c`** (both board `esp32-s3-devkitc-1`)
- Serial: **115200** baud over the board's CH343 USB-UART bridge
- `ARDUINO_USB_CDC_ON_BOOT=0` so serial/logs come out the CH343 port

To produce the merged single-file image yourself (same as the release `.bin`):

```bash
pio run -e supermini -t merge          # -> .pio/build/supermini/firmware-merged.bin (flash at 0x0)
pio run -e waveshare-28c -t merge      # -> .pio/build/waveshare-28c/firmware-merged.bin
```

### Regenerating the smooth font

The 480×480 build embeds `data/ui_font_30.vlw` (Noto Sans Bold, 30 px). Rebuild it with:

```bash
pip install pillow
python3 scripts/build_vlw_font.py --font "NotoSans[wdth,wght].ttf" \
    --variation Bold --size 30 --out data/ui_font_30.vlw
```

### Cutting a release

Tag a version and push it; the GitHub Actions workflow builds the firmware and publishes a release with the merged `.bin`:

```bash
git tag v1.1.0 && git push origin v1.1.0
```

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- Weather data by the [US National Weather Service](https://www.weather.gov/documentation/services-web-api) and [Open-Meteo](https://open-meteo.com); aircraft data by [adsb.fi](https://opendata.adsb.fi/)

## Credits & license

MIT. Original project © 2026 MatixYo — [ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar). ESP32-S3 port, touch, and weather page © 2026 TurboTime29. See [`LICENSE`](LICENSE).
