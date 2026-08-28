/**
 * Plane Radar — WiFi setup, then radar UI on the round display.
 * Board (panel, touch, pins) is selected at build time — see include/config.h.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "services/touch.h"
#include "services/weather.h"
#include "services/wifi_setup.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"
#include "ui/weather_display.h"

namespace {

enum class Page { Radar, Weather };

bool g_radar_visible = false;
Page g_page = Page::Radar;
unsigned long g_wifi_down_since = 0;
unsigned long g_last_reconnect_ms = 0;
unsigned long g_last_adsb_fetch_ms = 0;
unsigned long g_last_weather_fetch_ms = 0;

void showRadarIfConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    g_radar_visible = false;
    return;
  }
  ui::radarDisplayDraw();
  g_radar_visible = true;
}

/** Step through the range presets; zoom_in picks the next shorter range. */
void changeRange(bool zoom_in) {
  if (zoom_in) {
    ui::radar::rangePrev();
  } else {
    ui::radar::rangeNext();
  }
  char range_label[12];
  ui::radar::formatCurrentRing3Label(range_label, sizeof(range_label));
  Serial.printf("Range: %s (outer ~%.0f km)\n", range_label,
                ui::radar::rangeCurrent().outer_km);

  if (g_radar_visible && WiFi.status() == WL_CONNECTED) {
    ui::radarDisplayDraw();
  }
}

/** Confirms a range change on the panel: the label survives the repaint. */
void showRangeToast() {
  char label[12];
  ui::radar::formatCurrentRing3Label(label, sizeof(label));
  statusScreenToast(label);
}

void handleBootButton() {
  bootButtonPollLongPress();
  if (bootButtonConsumeTap()) {
    changeRange(false);
    showRangeToast();
  }
}

void fetchAndDrawAircraft() {
  const float fetch_km = ui::radar::fetchRadiusKm();
  if (!services::adsb::fetchUpdate(services::location::lat(),
                                   services::location::lon(), fetch_km)) {
    handleBootButton();
    return;
  }
  ui::radarDisplayRefreshAircraft();
  handleBootButton();
}

/** Fetch (if connected) and draw the weather page. */
void showWeather(bool force_fetch) {
  if (WiFi.status() == WL_CONNECTED &&
      (force_fetch || !services::weather::current().valid)) {
    services::weather::fetch(services::location::lat(),
                             services::location::lon());
    g_last_weather_fetch_ms = millis();
  }
  ui::weatherDisplayDraw();
}

void togglePage() {
  if (g_page == Page::Radar) {
    g_page = Page::Weather;
    g_radar_visible = false;  // stop radar from drawing over the weather page
    showWeather(true);
  } else {
    g_page = Page::Radar;
    g_radar_visible = false;  // force a fresh radar redraw
    showRadarIfConnected();
  }
}

/**
 * Tap anywhere toggles radar/weather. On the radar, pinching or swiping walks
 * the range presets: apart/right/up zooms in, together/left/down zooms out.
 */
void handleTouch() {
  using services::touch::Gesture;

  const Gesture gesture = services::touch::consume();
  if (gesture == Gesture::None) {
    return;
  }
  if (gesture == Gesture::Tap) {
    togglePage();
    return;
  }
  if (g_page != Page::Radar) {
    return;  // gestures only steer the radar
  }

  switch (gesture) {
    case Gesture::PinchOut:
    case Gesture::SwipeRight:
    case Gesture::SwipeUp:
      changeRange(true);
      break;
    case Gesture::PinchIn:
    case Gesture::SwipeLeft:
    case Gesture::SwipeDown:
      changeRange(false);
      break;
    default:
      break;
  }
  showRangeToast();
}

/**
 * Samples touch and BOOT independently of the main loop: an ADS-B fetch plus a
 * full repaint can block it for seconds, which used to swallow taps and replay
 * them later (the weather page appearing "by itself").
 */
void inputTask(void*) {
  for (;;) {
    services::touch::update();
    bootButtonSample();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Plane Radar — " BOARD_NAME);

  bootButtonInit();
  services::touch::init();
  displayInit();
  // On-screen boot info: version, board and the touch IC that answered, so the
  // hardware can be checked without hooking up a serial console.
  statusScreenBootInfo(services::touch::controllerName());
  xTaskCreatePinnedToCore(inputTask, "input", 3072, nullptr, 2, nullptr, 1);
  delay(2000);
  if (wifiShowsSetupScreenOnBoot()) {
    statusScreenPortal();
  }
  services::location::init();
  ui::radar::rangeInit();

  if (wifiSetupConnect()) {
    showRadarIfConnected();
  }
}

void loop() {
  handleBootButton();
  handleTouch();

  if (g_page == Page::Weather) {
    // Keep the weather page refreshed; retry sooner if we have no data yet.
    if (WiFi.status() == WL_CONNECTED) {
      const services::weather::Data& w = services::weather::current();
      const unsigned long since = millis() - g_last_weather_fetch_ms;
      if (since >= config::kWeatherFetchIntervalMs ||
          (!w.valid && since >= config::kWeatherRetryIntervalMs)) {
        showWeather(true);
      }
    }
    delay(10);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (g_radar_visible) {
      Serial.println("WiFi lost — will reconnect");
      g_radar_visible = false;
    }

    if (g_wifi_down_since == 0) {
      g_wifi_down_since = millis();
    }

    const unsigned long down_ms = millis() - g_wifi_down_since;
    if (down_ms >= config::kWifiDownGraceMs &&
        millis() - g_last_reconnect_ms >= config::kWifiReconnectIntervalMs) {
      g_last_reconnect_ms = millis();
      if (wifiReconnect()) {
        g_wifi_down_since = 0;
        showRadarIfConnected();
      }
    }
  } else {
    g_wifi_down_since = 0;
    if (!g_radar_visible) {
      showRadarIfConnected();
    } else if (millis() - g_last_adsb_fetch_ms >= config::kAdsbFetchIntervalMs) {
      g_last_adsb_fetch_ms = millis();
      fetchAndDrawAircraft();
    }
  }

  delay(10);
}
