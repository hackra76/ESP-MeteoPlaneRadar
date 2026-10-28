#include <Arduino.h>
#include <Wire.h>
#include "lvgl.h"
#include "TCA9554.h"
#include "Display_ST7701.h"
#include "Touch_CST820.h"
#include "Config.h"

#include "AsyncCore.h"
#include "UI.h"
#include "Watchdog.h"
#include "Settings.h"
#include "Buzzer.h"
#include "Outside.h"
#include "FlightStats.h"
#include "PrecipTracker.h"
#include "NightMode.h"
#include "Adsb.h"
#include <time.h>
#include "WiFiPortal.h"
#include "GeoIP.h"
#include "FinanceData.h"
#include "IssData.h"
#include "YouTubeData.h"
#include "KlipperData.h"
#include "PetBrain.h"
#include "DisasterAlert.h"
#include "WebConfig.h"
#include "ScreenPlanes.h"
#include "ScreenWeather.h"
#include "ScreenTactical.h"
#include "ScreenSonar.h"
#include "ScreenSettings.h"
#include "PetDrawer.h"
#include "QuickControl.h"
#include "QMI8658.h"
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <lwip/sockets.h>


// LVGL Display flushing callback
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  // Our display driver LCD_Flush function takes the whole framebuffer and pushes it via DMA.
  if (lv_display_flush_is_last(disp)) {
    LCD_Flush((const uint16_t*)px_map);
  }
  lv_display_flush_ready(disp);
}

// LVGL Touch input reading callback
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  TouchData t;
  Touch_Read(&t);
  
  if (t.points > 0) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = t.x;
    data->point.y = t.y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = t.x;
    data->point.y = t.y;
  }
}

static uint32_t my_tick_get_cb(void) {
  return millis();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("LVGL V9 Migration Phase 2 Started");

  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  Wire.setTimeOut(50);
  delay(50);
  TCA9554_Init();

  Backlight_Init();
  
  if (!ST7701_Init()) {
    Serial.println("Display initialization failed");
    while (1) delay(100);
  }

  Set_Backlight(60);

  if (!Touch_Init()) {
    Serial.println("Touch initialization failed");
  }

  // Initialize LVGL
  lv_init();
  lv_tick_set_cb(my_tick_get_cb);

  // 1. Create Display
  lv_display_t * disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, my_disp_flush);
  
  // Use the pre-allocated PSRAM framebuffers from our ST7701 driver
  void * buf1 = (void *)LCD_FrameBuffer(0);
  void * buf2 = (void *)LCD_FrameBuffer(1);
  
  // We use FULL mode so LVGL draws the entire screen to our full-screen buffers
  lv_display_set_buffers(disp, buf1, buf2, LCD_WIDTH * LCD_HEIGHT * 2, LV_DISPLAY_RENDER_MODE_FULL);

  // 2. Create Input Device
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_timer_set_period(lv_indev_get_read_timer(indev), 10);

  // 3. Initialize Master Screen UI
  UI_Init();
  
  Watchdog_Begin();
  Settings_Begin();
  Buzzer_Init();
  Outside_Init();
  FlightStats_Init();
  PrecipTracker_Init();
  Finance_Init();
  Iss_Init();
  YouTube_Init();
  Klipper_Init();
  PetBrain_Init();
  DisasterAlert_Init();

  if (QMI8658_Init()) {
    QMI8658_OnDoubleTap([]() {
      if (NightMode_IsUltraNightActive()) {
        NightMode_WakeTemporary(15000);
      }
      if (PetDrawer_IsOpen()) {
        if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_OVERHEAD);
        PetBrain_Feed();
        UI_InvalidateActiveScreen();
        return;
      }
      const int activeScr = UI_GetActiveScreen();
      if (activeScr != SCREEN_PLANES_I && activeScr != SCREEN_METEO_I &&
          activeScr != SCREEN_TACTICAL_I && activeScr != SCREEN_SONAR_I) {
        return;
      }
      if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
      Settings_ToggleLegends();
      UI_InvalidateActiveScreen();
      Serial.printf("IMU Gesture: Double-tap detected -> ShowLegends = %d\n", Settings_ShowLegends());
    });
  }

  WiFi_Begin();

  if (WiFi_IsConnected()) {
    GeoIP_DetectIfNeeded();
  }

  if (!WiFi_IsAP()) {
    UI_SwitchScreen(SCREEN_CLOCK_I);
  }
  
  // 4. Start Background Worker on Core 0
  Async_Begin();

  Serial.println("LVGL Setup Complete");
}

volatile const char* g_mainStage = "boot";
volatile uint32_t g_mainStageMs = 0;

void loop() {
  if (WebConfig_UpdateBusy()) {
    Watchdog_Feed();
    delay(20);
    return;
  }
  g_mainStage = "qmi"; g_mainStageMs = millis();
  QMI8658_Tick();

  g_mainStage = "wifi"; g_mainStageMs = millis();
  bool wasAP = WiFi_IsAP();
  WiFi_Loop();
  
  if (wasAP && !WiFi_IsAP()) {
    UI_SwitchScreen(SCREEN_CLOCK_I);
  }

  g_mainStage = "webconfig_take"; g_mainStageMs = millis();
  int reqScreen = WebConfig_TakeScreen();
  if (reqScreen >= 0) {
      UI_SwitchScreen(reqScreen);
  }
  
  int reqStep = WebConfig_TakeScreenStep();
  if (reqStep != 0) {
      UI_SwitchScreenStep(reqStep);
  }

  int reqRange = WebConfig_TakeRangeStep();
  if (reqRange != 0) {
      int cur = UI_GetActiveScreen();
      if      (cur == SCREEN_PLANES_I)   ScreenPlanes_ChangeRange(reqRange);
      else if (cur == SCREEN_METEO_I)    ScreenWeather_ChangeRange(reqRange);
      else if (cur == SCREEN_TACTICAL_I) ScreenTactical_ChangeRange(reqRange);
      else if (cur == SCREEN_SONAR_I)    ScreenSonar_ChangeRange(reqRange);
      UI_InvalidateActiveScreen();
  }

  if (WebConfig_TakeSelectPlane()) {
      if (UI_GetActiveScreen() != SCREEN_PLANES_I) {
          UI_SwitchScreen(SCREEN_PLANES_I);
      }
      ScreenPlanes_SelectFirst();
      UI_InvalidateActiveScreen();
  }

  if (WebConfig_TakePetToggle()) {
      PetDrawer_Toggle();
  }

  if (WebConfig_TakeQuickControlToggle()) {
      QuickControl_Toggle();
  }

  if (WebConfig_TakeRedraw()) {
      UI_InvalidateActiveScreen();
  }

  if (WebConfig_TakeDisplayResync()) {
      LCD_Restart();
  }

  if (WebConfig_WantsRestart()) {
      Serial.println("Settings changed, restarting...");
      delay(200);
      Safe_Restart();
  }

  g_mainStage = "buzzer"; g_mainStageMs = millis();
  Buzzer_Tick();
  g_mainStage = "autorot"; g_mainStageMs = millis();
  UI_AutoRotate_Tick();
  g_mainStage = "night"; g_mainStageMs = millis();
  NightMode_Tick();
  g_mainStage = "settings"; g_mainStageMs = millis();
  Settings_Tick();
  g_mainStage = "disaster"; g_mainStageMs = millis();
  DisasterAlert_Tick();
  g_mainStage = "flightstats"; g_mainStageMs = millis();
  FlightStats_Tick();

  if (ScreenSettings_WantsWifiReset()) {
    ScreenSettings_ClearWifiReset();
    WiFi_Reset();
  }

  // Periodic I2C expander register verification & display VSYNC watchdog
  {
    static unsigned long s_lastExpCheck = 0;
    if (millis() - s_lastExpCheck >= EXPANDER_CHECK_MS) {
      s_lastExpCheck = millis();
      TCA9554_Verify();
    }
#if DISPLAY_WD
    static unsigned long s_lastVsyncCheck = 0;
    static uint32_t s_prevVsync = 0;
    if (millis() - s_lastVsyncCheck >= DISPLAY_WD_DEAD_MS) {
      s_lastVsyncCheck = millis();
      uint32_t curVsync = LCD_VsyncCount();
      if (s_prevVsync != 0 && curVsync == s_prevVsync) {
        Serial.println("Display WD: VSYNC stalled, resyncing RGB panel...");
        LCD_Restart();
      }
      s_prevVsync = LCD_VsyncCount();
    }
#endif
  }
  
  // Smart auto-dimming check (30 minutes of inactivity)
  g_mainStage = "dimming"; g_mainStageMs = millis();
  uint32_t inactive_ms = lv_display_get_inactive_time(lv_display_get_default());
  bool is_idle = (inactive_ms > 30 * 60 * 1000UL) && 
                 (ADSB_Count() == 0) && 
                 (!PrecipTracker_IsApproaching()) &&
                 (!DisasterAlert_HasActiveAlert());
  NightMode_SetIdle(is_idle);

  // PSRAM Defragmentation Watch (runs around 3:00 AM)
  g_mainStage = "maint"; g_mainStageMs = millis();
  static unsigned long last_maint = 0;
  if (millis() - last_maint > 300000UL) { // Check every 5 minutes
      last_maint = millis();
      time_t now = time(nullptr);
      struct tm lt;
      localtime_r(&now, &lt);
      if (lt.tm_hour == 3 && is_idle) { // 3:00 AM - 3:59 AM
          size_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
          if (largest_psram < 500 * 1024) { // < 500 KB block size
              Serial.printf("Maintenance: PSRAM fragmented (Largest block: %zu bytes). Restarting gracefully.\n", largest_psram);
              delay(100);
              Safe_Restart();
          }
      }
  }

  // Diagnostic heartbeat (every 2 s): Core 1 liveness, heap, sockets, WiFi, Core 0 worker stage.
  g_mainStage = "hb"; g_mainStageMs = millis();
  {
    static unsigned long s_lastHb = 0;
    if (millis() - s_lastHb >= 15000) {
      s_lastHb = millis();
      int socks = 0;
      for (int fd = LWIP_SOCKET_OFFSET; fd < LWIP_SOCKET_OFFSET + MEMP_NUM_NETCONN; fd++) {
        if (lwip_fcntl(fd, F_GETFL, 0) >= 0) socks++;
      }
      char dbg[64];
      Async_DebugInfo(dbg, sizeof(dbg));
      Serial.printf("[HB] t=%lus wifi=%d rssi=%d int=%u blk=%u min=%u psram=%u sock=%d %s\n",
                    millis() / 1000UL, (int)WiFi.status(), (int)WiFi.RSSI(),
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                    (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                    (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM), socks, dbg);
    }
  }

  g_mainStage = "lvgl"; g_mainStageMs = millis();
  lv_timer_handler();
  g_mainStage = "wdt"; g_mainStageMs = millis();
  Watchdog_Feed();
  g_mainStage = "delay"; g_mainStageMs = millis();
  delay(5);
  g_mainStage = "idle"; g_mainStageMs = millis();
}
