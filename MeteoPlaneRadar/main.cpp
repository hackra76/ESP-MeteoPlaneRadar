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
#include "WiFiPortal.h"
#include "GeoIP.h"
#include "FinanceData.h"
#include "IssData.h"
#include "YouTubeData.h"
#include "PetBrain.h"
#include "WebConfig.h"
#include "ScreenPlanes.h"
#include "ScreenWeather.h"
#include "ScreenTactical.h"
#include "ScreenSonar.h"
#include "PetDrawer.h"
#include "QuickControl.h"
#include "QMI8658.h"

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
  PetBrain_Init();

  if (QMI8658_Init()) {
    QMI8658_OnDoubleTap([]() {
      if (PetDrawer_IsOpen()) {
        if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_OVERHEAD);
        PetBrain_Feed();
        UI_InvalidateActiveScreen();
        return;
      }
      if (UI_GetActiveScreen() == SCREEN_CLOCK_I) {
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

void loop() {
  QMI8658_Tick();

  bool wasAP = WiFi_IsAP();
  WiFi_Loop();
  
  if (wasAP && !WiFi_IsAP()) {
    UI_SwitchScreen(SCREEN_CLOCK_I);
  }

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

  if (WebConfig_WantsRestart()) {
      Serial.println("Settings changed, restarting...");
      delay(200);
      Safe_Restart();
  }

  Buzzer_Tick();
  UI_AutoRotate_Tick();
  lv_timer_handler();
  Watchdog_Feed();
  delay(5);
}
