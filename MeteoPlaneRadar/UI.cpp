#include "UI.h"
#include "PetDrawer.h"
#include "PetBrain.h"
#include "QuickControl.h"
#include "Config.h"
#include "ScreenPlanes.h"
#include "Outside.h"
#include "Display_ST7701.h"
#include "Layout.h"
#include "Lang.h"
#include "Settings.h"
#include "AsyncCore.h"
#include "Buzzer.h"
#include <math.h>
#include "qrcode.h"
#include "ADSB.h"
#include "Route.h"
#include "PlanePhoto.h"
#include "AircraftType.h"
#include "ScreenWeather.h"
#include "ScreenTactical.h"
#include "ScreenSettings.h"
#include "ScreenClock.h"
#include "ScreenForecast.h"
#include "ScreenFinance.h"
#include "ScreenIss.h"
#include "ScreenYouTube.h"
#include "ScreenInfo.h"
#include "ScreenSonar.h"
#include "RainViewer.h"
#include "CHMU.h"
#include "SHMU.h"

static int s_activeScreen = SCREEN_CLOCK_I;
static lv_obj_t* s_wifiScreen = nullptr;
static bool s_wifiModeAP = false;
static String s_wifiSSID = "";
static String s_wifiPass = "";
static unsigned long s_lastAutoRotateMs = 0;
static unsigned long s_lastUserInteractionMs = 0;

static void wifi_screen_draw_cb(lv_event_t * e) {
    lv_layer_t * layer = lv_event_get_layer(e);
    gfx->setLayer(layer);

    if (s_wifiModeAP) {
        const uint8_t lang = Lang_Get();
        UI_TextCentered("MeteoPlaneRadar", 34, C_CYAN, 2);
        UI_TextCentered("H4CKR4", 58, C_GRAY, 1);
        const char* scanTxt = (lang == LANG_EN) ? "Scan with your phone:"
                            : ((lang == LANG_SK) ? "Naskenuj mobilom:" : "Naskenuj mobilem:");
        UI_TextCentered(scanTxt, 78, C_GRAY, 1);

        const int qrSize = 190;
        UI_DrawWifiQR(s_wifiSSID.c_str(), s_wifiPass.c_str(), true, (LCD_WIDTH - qrSize) / 2, 98, qrSize);

        UI_TextCentered(s_wifiSSID.c_str(), 300, C_WHITE, 1);
        const char* openTxt = (lang == LANG_EN) ? "no password  |  then open 192.168.4.1"
                            : ((lang == LANG_SK) ? "bez hesla  |  potom otvor 192.168.4.1" : "bez hesla  |  pak otevri 192.168.4.1");
        UI_TextCentered(openTxt, 322, C_GRAY, 1);
        const char* waitTxt = (lang == LANG_EN) ? "Waiting for your network..."
                            : ((lang == LANG_SK) ? "Cakam na tvoju siet..." : "Cekam na tvoji sit...");
        UI_TextCentered(waitTxt, 430, C_GREEN, 1);
    } else {
        const uint8_t lang = Lang_Get();
        const char* connTxt = (lang == LANG_EN) ? "Connecting to WiFi..."
                            : ((lang == LANG_SK) ? "Pripajam k WiFi..." : "Pripojuji k WiFi...");
        UI_TextCentered(connTxt, LCD_HEIGHT / 2 - 20, C_WHITE, 2);
        if (s_wifiSSID.length() > 0) UI_TextCentered(s_wifiSSID.c_str(), LCD_HEIGHT / 2 + 12, C_CYAN, 2);
    }
    
    gfx->setLayer(nullptr);
}

void UI_ShowConnecting(const char* ssid) {
    if (!s_wifiScreen) {
        s_wifiScreen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(s_wifiScreen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(s_wifiScreen, LV_OPA_COVER, 0);
        lv_obj_add_event_cb(s_wifiScreen, wifi_screen_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    }
    s_wifiModeAP = false;
    s_wifiSSID = ssid ? ssid : "";
    lv_screen_load(s_wifiScreen);
    lv_timer_handler();
}

void UI_ShowAP(const char* ssid, const char* pass) {
    if (!s_wifiScreen) {
        s_wifiScreen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(s_wifiScreen, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(s_wifiScreen, LV_OPA_COVER, 0);
        lv_obj_add_event_cb(s_wifiScreen, wifi_screen_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    }
    s_wifiModeAP = true;
    s_wifiSSID = ssid ? ssid : "";
    s_wifiPass = pass ? pass : "";
    lv_screen_load(s_wifiScreen);
    lv_timer_handler();
}
static lv_obj_t* screens[SCREEN_N];
static Arduino_GFX _gfx_instance;
Arduino_GFX* gfx = &_gfx_instance;

// ---------------------------------------------------------------------------
// Self-contained swipe recogniser
//
// We do NOT rely on LV_EVENT_GESTURE: LVGL fires it on every indev read cycle
// while the threshold is exceeded, which turns one long drag into 2-3 calls
// to UI_SwitchScreenStep and skips screens. Instead we track PRESSED/RELEASED
// ourselves and fire each action exactly once per physical gesture.
// ---------------------------------------------------------------------------

// Touch state for our own gesture tracker
static lv_point_t  s_touchStart    = {0, 0};
static bool        s_touchActive   = false;   // finger is currently down
static bool        s_gestureFired  = false;   // action already dispatched for this touch
static unsigned long s_screenSwitchMs = 0;    // timestamp of last screen switch (cooldown)
static unsigned long s_lastGestureMs   = 0;    // timestamp of last gesture dispatch

// Double-tap state
static unsigned long s_lastTapMs   = 0;
static lv_point_t  s_lastTapPoint  = {0, 0};

// Swipe thresholds
#define SWIPE_MIN_PX       40     // minimum travel to be considered a swipe
#define SWIPE_DIR_RATIO    1.3f   // dominant axis must be 1.3x the cross axis
#define SWIPE_TOP_ZONE_Y   180    // y <= this -> top-edge for pull-down
#define SWIPE_BOT_ZONE_Y   350    // y >= this -> bottom-edge for pull-up
#define SCREEN_SWITCH_COOLDOWN_MS 350  // ignore new screen-switch within this window

// Zoom control button geometry on the right side of the display
#define ZOOM_BTN_CX       430
#define ZOOM_BTN_Y_PLUS   185
#define ZOOM_BTN_Y_MINUS  295
#define ZOOM_BTN_R        24
#define ZOOM_BTN_HIT_R    36
#define ZOOM_TIMEOUT_MS   3500UL

static bool s_zoomVisible = false;
static unsigned long s_zoomVisibleMs = 0;

bool UI_IsZoomScreen(int screen) {
    if (screen == SCREEN_PLANES_I) {
        return !ScreenPlanes_DetailOpen() && !UI_IsPhotoFullscreen();
    }
    if (screen == SCREEN_TACTICAL_I) {
        return !ScreenTactical_DetailOpen() && !UI_IsPhotoFullscreen();
    }
    return (screen == SCREEN_METEO_I || screen == SCREEN_SONAR_I);
}

bool UI_IsZoomControlsVisible() {
    return s_zoomVisible && UI_IsZoomScreen(s_activeScreen);
}

void UI_ShowZoomControls() {
    if (!UI_IsZoomScreen(s_activeScreen)) return;
    s_zoomVisible = true;
    s_zoomVisibleMs = millis();
    UI_InvalidateActiveScreen();
}

void UI_HideZoomControls() {
    if (s_zoomVisible) {
        s_zoomVisible = false;
        UI_InvalidateActiveScreen();
    }
}

void UI_ZoomTick() {
    if (s_zoomVisible && (millis() - s_zoomVisibleMs >= ZOOM_TIMEOUT_MS)) {
        s_zoomVisible = false;
        UI_InvalidateActiveScreen();
    }
}

static void executeZoom(int dir) {
    if (s_activeScreen == SCREEN_PLANES_I) {
        ScreenPlanes_ChangeRange(dir);
    } else if (s_activeScreen == SCREEN_METEO_I) {
        ScreenWeather_ChangeRange(dir);
    } else if (s_activeScreen == SCREEN_TACTICAL_I) {
        ScreenTactical_ChangeRange(dir);
    } else if (s_activeScreen == SCREEN_SONAR_I) {
        ScreenSonar_ChangeRange(dir);
    }
    if (s_activeScreen != SCREEN_SONAR_I && Settings_BuzzerTouch()) {
        Buzzer_Play(BEEP_CLICK);
    }
}

void UI_DrawZoomControls() {
    if (!UI_IsZoomControlsVisible()) return;

    const int cx = ZOOM_BTN_CX;
    const int yPlus = ZOOM_BTN_Y_PLUS;
    const int yMinus = ZOOM_BTN_Y_MINUS;
    const int r = ZOOM_BTN_R;

    // Translucent dark glass backdrop (70% opacity)
    uint16_t glassBg = RGB565(14, 22, 34);
    uint16_t glassRim = RGB565(60, 140, 210);
    uint16_t symCol = C_WHITE;

    // '+' Button (Zoom In)
    gfx->fillCircleOpa(cx, yPlus, r, glassBg, LV_OPA_70);
    gfx->drawCircle(cx, yPlus, r, glassRim);
    // Draw '+' symbol (14px wide, 2px thick)
    gfx->drawFastHLine(cx - 7, yPlus - 1, 15, symCol);
    gfx->drawFastHLine(cx - 7, yPlus,     15, symCol);
    gfx->drawFastVLine(cx - 1, yPlus - 7, 15, symCol);
    gfx->drawFastVLine(cx,     yPlus - 7, 15, symCol);

    // '-' Button (Zoom Out)
    gfx->fillCircleOpa(cx, yMinus, r, glassBg, LV_OPA_70);
    gfx->drawCircle(cx, yMinus, r, glassRim);
    // Draw '-' symbol (14px wide, 2px thick)
    gfx->drawFastHLine(cx - 7, yMinus - 1, 15, symCol);
    gfx->drawFastHLine(cx - 7, yMinus,     15, symCol);
}

bool UI_IsSwipeActive() {
    return s_gestureFired || (millis() - s_lastGestureMs < 450) || (millis() - s_screenSwitchMs < 450);
}

static void handleRelease(int startX, int startY, int endX, int endY) {
    int dx    = endX - startX;
    int dy    = endY - startY;
    int absDx = abs(dx);
    int absDy = abs(dy);

    // --- Tap (no significant movement) --------------------------------------
    if (absDx < SWIPE_MIN_PX && absDy < SWIPE_MIN_PX) {
        // Zoom button interaction or wake-up on zoomable screens
        if (UI_IsZoomScreen(s_activeScreen)) {
            if (s_zoomVisible) {
                int dPlusX = endX - ZOOM_BTN_CX;
                int dPlusY = endY - ZOOM_BTN_Y_PLUS;
                if ((dPlusX * dPlusX + dPlusY * dPlusY) <= (ZOOM_BTN_HIT_R * ZOOM_BTN_HIT_R)) {
                    executeZoom(-1); // Zoom In
                    s_zoomVisibleMs = millis();
                    s_lastGestureMs = millis();
                    UI_InvalidateActiveScreen();
                    return;
                }

                int dMinusX = endX - ZOOM_BTN_CX;
                int dMinusY = endY - ZOOM_BTN_Y_MINUS;
                if ((dMinusX * dMinusX + dMinusY * dMinusY) <= (ZOOM_BTN_HIT_R * ZOOM_BTN_HIT_R)) {
                    executeZoom(1); // Zoom Out
                    s_zoomVisibleMs = millis();
                    s_lastGestureMs = millis();
                    UI_InvalidateActiveScreen();
                    return;
                }

                // Tap in the right-side activation corridor keeps buttons awake
                if (endX >= 370 && endY >= 130 && endY <= 350) {
                    s_zoomVisibleMs = millis();
                    s_lastGestureMs = millis();
                    return;
                }
            } else {
                // If buttons were hidden and tap is in the right-side corridor, wake them up
                if (endX >= 370 && endY >= 130 && endY <= 350) {
                    s_zoomVisible = true;
                    s_zoomVisibleMs = millis();
                    s_lastGestureMs = millis();
                    if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
                    UI_InvalidateActiveScreen();
                    return;
                }
            }
        }

        // Single tap: buzzer feedback
        if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);

        // Double-tap detection
        unsigned long now = millis();
        int dtx = abs(endX - s_lastTapPoint.x);
        int dty = abs(endY - s_lastTapPoint.y);
        if ((now - s_lastTapMs >= 50) && (now - s_lastTapMs <= 400)
                && dtx <= 45 && dty <= 45) {
            s_lastTapMs = 0;
            if (PetDrawer_IsOpen()) {
                if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_OVERHEAD);
                PetBrain_Feed();
                UI_InvalidateActiveScreen();
                return;
            }
            if (s_activeScreen != SCREEN_CLOCK_I) {
                Settings_ToggleLegends();
                UI_InvalidateActiveScreen();
            }
            return;
        }
        s_lastTapMs    = now;
        s_lastTapPoint = {(lv_coord_t)endX, (lv_coord_t)endY};
        return;
    }

    // --- Swipe: classify direction ------------------------------------------
    bool isHorizontal = (absDx >= SWIPE_MIN_PX) && (absDx >= (int)(absDy * SWIPE_DIR_RATIO));
    bool isVertical   = (absDy >= (SWIPE_MIN_PX + 5)) && (absDy >= (int)(absDx * SWIPE_DIR_RATIO));
    if (!isHorizontal && !isVertical) return; // diagonal - ignore

    // Settings brightness slider row: protect from horizontal swipes
    if (s_activeScreen == SCREEN_SETTINGS_I && isHorizontal
            && startY >= 90 && startY <= 165) return;

    if (isVertical) {
        s_lastGestureMs = millis();
        // Pull-down (top edge)
        if (startY <= SWIPE_TOP_ZONE_Y && dy > 0) {
            QuickControl_Open();
            return;
        }
        // Pull-up (bottom edge) -> Pet Drawer
        if (startY >= SWIPE_BOT_ZONE_Y && dy < 0) {
            if (Settings_PetEnabled()) PetDrawer_Open();
            return;
        }
        return;
    }

    if (isHorizontal) {
        s_lastGestureMs = millis();
        s_zoomVisible = false;
        // Close aircraft detail first if open
        if (s_activeScreen == SCREEN_PLANES_I && ScreenPlanes_DetailOpen()) {
            ScreenPlanes_CloseDetail();
            UI_InvalidateActiveScreen();
            return;
        }
        if (s_activeScreen == SCREEN_TACTICAL_I && ScreenTactical_DetailOpen()) {
            ScreenTactical_CloseDetail();
            UI_InvalidateActiveScreen();
            return;
        }

        // Cooldown: reject if we just switched
        if (millis() - s_screenSwitchMs < SCREEN_SWITCH_COOLDOWN_MS) return;

        s_screenSwitchMs = millis();
        if (Settings_BuzzerTouch()) Buzzer_Play(BEEP_CLICK);
        UI_SwitchScreenStep(dx < 0 ? 1 : -1);
    }
}

static void global_screen_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t * indev = lv_indev_active();
    if (!indev) return;

    if (code == LV_EVENT_PRESSED) {
        UI_NotifyInteraction();
        // If s_touchActive is already true, this PRESSED was synthetically generated
        // by lv_screen_load() switching screens mid-swipe. Ignore it completely to
        // prevent resetting s_gestureFired and firing a second action.
        if (s_touchActive) return;
        lv_indev_get_point(indev, &s_touchStart);
        s_touchActive  = true;
        s_gestureFired = false;

        // Wake zoom buttons immediately on touch in right activation zone
        if (UI_IsZoomScreen(s_activeScreen) && s_touchStart.x >= 370 && s_touchStart.y >= 130 && s_touchStart.y <= 350) {
            if (!s_zoomVisible) {
                s_zoomVisible = true;
                s_zoomVisibleMs = millis();
                UI_InvalidateActiveScreen();
            } else {
                s_zoomVisibleMs = millis();
            }
        }
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        // Real-time swipe: fire the gesture action ONCE as soon as direction is clear.
        if (!s_touchActive || s_gestureFired) return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);
        int absDx = abs(p.x - s_touchStart.x);
        int absDy = abs(p.y - s_touchStart.y);

        // Direction check: dominant axis must be at least 1.3x cross axis
        bool isH = (absDx >= 25) && (absDx >= (int)(absDy * SWIPE_DIR_RATIO));
        bool isV = (absDy >= 30) && (absDy >= (int)(absDx * SWIPE_DIR_RATIO));
        // Vertical gestures only valid from top edge (pull-down) or bottom edge (pull-up)
        bool isVValid = isV && ((s_touchStart.y <= SWIPE_TOP_ZONE_Y && (p.y - s_touchStart.y) > 0) ||
                                (s_touchStart.y >= SWIPE_BOT_ZONE_Y && (p.y - s_touchStart.y) < 0));
        if (!isH && !isVValid) return; // diagonal, centre vertical, or too short - keep waiting

        // Fire threshold: 40px on horizontal, 45px on vertical
        if (isH && absDx < SWIPE_MIN_PX) return;
        if (isVValid && absDy < (SWIPE_MIN_PX + 5)) return;

        s_gestureFired = true; // lock: won't fire again until next PRESSED
        s_lastGestureMs = millis();
        handleRelease(s_touchStart.x, s_touchStart.y, p.x, p.y);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (!s_touchActive) return;
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        bool wasFired = s_gestureFired;
        s_touchActive  = false;
        s_gestureFired = false;
        if (wasFired) {
            s_lastGestureMs = millis();
        }

        if (!wasFired) {
            // Finger lifted without crossing the swipe threshold: treat as tap
            handleRelease(s_touchStart.x, s_touchStart.y, p.x, p.y);
        }
        // If gesture already fired during PRESSING, nothing more to do.
        return;
    }

    // We no longer use LV_EVENT_CLICKED or LV_EVENT_GESTURE - both are
    // replaced by the PRESSED/PRESSING/RELEASED state machine above.
}

void UI_Init() {
    for(int i = 0; i < SCREEN_N; i++) {
        screens[i] = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(screens[i], lv_color_black(), 0);
        lv_obj_set_style_bg_opa(screens[i], LV_OPA_COVER, 0);
        lv_obj_set_scrollable(screens[i], false);
        lv_obj_set_clickable(screens[i], true);
        lv_obj_add_event_cb(screens[i], global_screen_event_cb, LV_EVENT_PRESSED,    NULL);
        lv_obj_add_event_cb(screens[i], global_screen_event_cb, LV_EVENT_PRESSING,   NULL);
        lv_obj_add_event_cb(screens[i], global_screen_event_cb, LV_EVENT_RELEASED,   NULL);
        lv_obj_add_event_cb(screens[i], global_screen_event_cb, LV_EVENT_PRESS_LOST, NULL);
        
        if (i == SCREEN_PLANES_I) {
            ScreenPlanes_Init(screens[i]);
        } else if (i == SCREEN_METEO_I) {
            ScreenWeather_Init(screens[i]);
        } else if (i == SCREEN_TACTICAL_I) {
            ScreenTactical_Init(screens[i]);
        } else if (i == SCREEN_SETTINGS_I) {
            ScreenSettings_Init(screens[i]);
        } else if (i == SCREEN_CLOCK_I) {
            ScreenClock_Init(screens[i]);
        } else if (i == SCREEN_FORECAST_I) {
            ScreenForecast_Init(screens[i]);
        } else if (i == SCREEN_FINANCE_I) {
            ScreenFinance_Init(screens[i]);
        } else if (i == SCREEN_ISS_I) {
            ScreenIss_Init(screens[i]);
        } else if (i == SCREEN_YOUTUBE_I) {
            ScreenYouTube_Init(screens[i]);
        } else if (i == SCREEN_INFO_I) {
            ScreenInfo_Init(screens[i]);
        } else if (i == SCREEN_SONAR_I) {
            ScreenSonar_Init(screens[i]);
        } else {
            lv_obj_t *label = lv_label_create(screens[i]);
            lv_label_set_text_fmt(label, "Screen %d", i);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        }
    }

    QuickControl_Init();
    PetDrawer_Init();
}

void UI_SwitchScreen(int screenIdx) {
    if (screenIdx < 0 || screenIdx >= SCREEN_N) return;
    int prevScreen = s_activeScreen;
    if (prevScreen != screenIdx) {
        // Free screen buffers and downloaded data no longer needed for inactive screens
        if (prevScreen == SCREEN_METEO_I && screenIdx != SCREEN_METEO_I) {
            ScreenWeather_FreeBuffers();
        }
        if (prevScreen == SCREEN_TACTICAL_I && screenIdx != SCREEN_TACTICAL_I) {
            ScreenTactical_FreeBuffers();
        }
        if (screenIdx != SCREEN_METEO_I && screenIdx != SCREEN_TACTICAL_I) {
            // Neither screen is a radar screen -> free all radar buffers immediately
            RainViewer_FreeBuffers();
            CHMU_FreeBuffers();
            SHMU_FreeBuffers();
            ScreenWeather_FreeBuffers();
            ScreenTactical_FreeBuffers();
        }
        if ((prevScreen == SCREEN_PLANES_I || prevScreen == SCREEN_TACTICAL_I) &&
            screenIdx != SCREEN_PLANES_I && screenIdx != SCREEN_TACTICAL_I) {
            ScreenPlanes_CloseDetail();
            ScreenTactical_CloseDetail();
            PlanePhoto_ClearCache();
            Route_ClearQueue();
            Route_Clear();
        }
        if (screenIdx == SCREEN_METEO_I) {
            ScreenPlanes_CloseDetail();
            ScreenTactical_CloseDetail();
            PlanePhoto_ClearCache();
            Route_ClearQueue();
            Route_Clear();
        }
    }
    s_activeScreen = screenIdx;
    s_zoomVisible = false;
    s_lastAutoRotateMs = millis();
    Settings_SetScreen(screenIdx);
    Async_SetActiveScreen(screenIdx);

    // Call screen enter handlers
    if (screenIdx == SCREEN_PLANES_I) {
        ScreenPlanes_Enter();
    } else if (screenIdx == SCREEN_METEO_I) {
        ScreenWeather_Enter();
    } else if (screenIdx == SCREEN_TACTICAL_I) {
        ScreenTactical_Enter();
    } else if (screenIdx == SCREEN_SONAR_I) {
        ScreenSonar_Enter();
    } else if (screenIdx == SCREEN_SETTINGS_I) {
        ScreenSettings_Enter();
    }

    lv_screen_load(screens[screenIdx]);
    lv_obj_invalidate(screens[screenIdx]);
}

void UI_InvalidateActiveScreen() {
    if (s_activeScreen >= 0 && s_activeScreen < SCREEN_N && screens[s_activeScreen]) {
        lv_obj_invalidate(screens[s_activeScreen]);
    }
}

void UI_SetScreensHidden(bool hidden) {
    if (s_activeScreen >= 0 && s_activeScreen < SCREEN_N && screens[s_activeScreen]) {
        if (hidden) {
            lv_obj_set_hidden(screens[s_activeScreen], true);
        } else {
            lv_obj_set_hidden(screens[s_activeScreen], false);
            lv_obj_invalidate(screens[s_activeScreen]);
        }
    }
}

void UI_NotifyInteraction() {
    s_lastUserInteractionMs = millis();
}

void UI_SwitchScreenStep(int step) {
    if (step == 0) return;
    int next = s_activeScreen;
    for (int guard = 0; guard < SCREEN_N; guard++) {
        next = (next + step + SCREEN_N) % SCREEN_N;
        if (Settings_ScreenEnabled(next)) break;
    }
    UI_SwitchScreen(next);
}

void UI_AutoRotate_Tick() {
    UI_ZoomTick();

    uint16_t rotSec = Settings_AutoRotateSec();
    if (rotSec == 0) return;

    // Do not auto-rotate during WiFi AP portal or photo modal
    if (s_wifiModeAP || UI_IsPhotoFullscreen()) return;

    // Do not auto-rotate if QuickControl or PetDrawer are open
    if (QuickControl_IsOpen() || PetDrawer_IsOpen()) return;

    // Do not auto-rotate if on Settings screen
    if (s_activeScreen >= SCREEN_SETTINGS_I) return;

    // Do not auto-rotate if an aircraft detail modal card is open
    if (s_activeScreen == SCREEN_PLANES_I && ScreenPlanes_DetailOpen()) return;
    if (s_activeScreen == SCREEN_TACTICAL_I && ScreenTactical_DetailOpen()) return;

    // Do not auto-rotate while a touch/gesture is active
    if (s_touchActive || UI_IsSwipeActive()) return;

    unsigned long now = millis();

    // Respect user touch pause: hold on current screen for at least rotSec after user interacts
    if (s_lastUserInteractionMs > 0 && (now - s_lastUserInteractionMs < (unsigned long)rotSec * 1000UL)) {
        s_lastAutoRotateMs = now;
        return;
    }

    if (s_lastAutoRotateMs == 0) {
        s_lastAutoRotateMs = now;
        return;
    }

    if (now - s_lastAutoRotateMs >= (unsigned long)rotSec * 1000UL) {
        s_lastAutoRotateMs = now;
        // Advance to next enabled data screen, skipping settings
        int next = s_activeScreen;
        for (int guard = 0; guard < SCREEN_N; guard++) {
            next = (next + 1) % SCREEN_N;
            if (next != SCREEN_SETTINGS_I && Settings_ScreenEnabled(next)) break;
        }
        if (next != s_activeScreen && next != SCREEN_SETTINGS_I && Settings_ScreenEnabled(next)) {
            UI_SwitchScreen(next);
        }
    }
}

int UI_GetActiveScreen() {
    return s_activeScreen;
}

lv_obj_t* UI_GetScreenObj(int screenIdx) {
    if (screenIdx >= 0 && screenIdx < SCREEN_N) return screens[screenIdx];
    return nullptr;
}

void UI_TextCenteredIn(const char* text, int x, int w, int cy,
                       uint16_t color, uint8_t size) {
  Font_DrawCenteredIn(text, x, w, cy, color, size);
}

void UI_TextCenteredBox(const char* text, int x, int y, int w, int h,
                        uint16_t color, uint8_t size) {
  Font_DrawCenteredBox(text, x, y, w, h, color, size);
}

void UI_TextCentered(const char* text, int cy, uint16_t color, uint8_t size) {
  Font_DrawCentered(text, LCD_WIDTH / 2, cy, color, size);
}

void UI_Text(const char* text, int x, int y, uint16_t color, uint8_t size) {
  Font_Draw(text, x, y, color, size);
}

static int UI_ChordHalfWidth(int y) {
  const int R = LCD_WIDTH / 2 - 2;
  long dy = (long)y - LCD_HEIGHT / 2;
  long d2 = (long)R * R - dy * dy;
  if (d2 <= 0) return 0;
  return (int)sqrtf((float)d2);
}

void UI_DrawStatusLine(int cy) {
  char txt[OUTSIDE_TEXT_MAX];
  Outside_StatusText(txt, sizeof(txt));
  if (!txt[0]) return;                      // nothing known yet - leave it empty

  int16_t tw = Font_TextWidth(txt, 2);
  int room = 2 * UI_ChordHalfWidth(cy + 16) - 8;
  if (tw > room) return;

  // Unified rounded pill matching all radar screens
  int pillW = tw + 16;
  int pillH = 22;
  int pillX = LCD_WIDTH / 2 - pillW / 2;
  int pillY = cy - 3;
  gfx->fillRoundRect(pillX, pillY, pillW, pillH, 6, C_BLACK);
  UI_TextCenteredBox(txt, pillX, pillY, pillW, pillH, C_WHITE, 2);
}

void UI_DrawWifiQR(const char* ssid, const char* password, bool open,
                   int x, int y, int size_px) {
  // WiFi QR payload
  String payload = "WIFI:T:";
  payload += open ? "nopass" : "WPA";
  payload += ";S:"; payload += ssid; payload += ";";
  if (!open) { payload += "P:"; payload += password; payload += ";"; }
  payload += ";";

  uint8_t version = 3;
  if (payload.length() > 60) version = 5;
  if (payload.length() > 100) version = 7;

  QRCode qr;
  uint8_t buf[qrcode_getBufferSize(7)];
  if (qrcode_initText(&qr, buf, version, ECC_MEDIUM, payload.c_str()) != 0) return;

  int modules = qr.size;
  int scale = size_px / (modules + 2);
  if (scale < 1) return;
  int qrPix = (modules + 2) * scale;

  gfx->fillRect(x, y, qrPix, qrPix, C_WHITE);
  int off = x + scale, offY = y + scale;
  for (int my = 0; my < modules; my++) {
    for (int mx = 0; mx < modules; mx++) {
      if (qrcode_getModule(&qr, mx, my)) {
        gfx->fillRect(off + mx * scale, offY + my * scale, scale, scale, C_BLACK);
      }
    }
  }
}

void UI_DrawRangeIndicator(const char* text, int activeIdx, int totalCount, bool showText) {
  if (totalCount <= 0) return;

  const int dotGap = 20;
  const int dotR = 4;
  const int dotY = LY_RANGE_DOTS;
  const int totalW = (totalCount - 1) * dotGap;
  const int startX = LCD_WIDTH / 2 - totalW / 2;

  if (showText && text && text[0]) {
    int tw = Font_TextWidth(text, 2);
    int pillW = max(tw + 20, totalW + 2 * (dotR + 8));
    int pillX = LCD_WIDTH / 2 - pillW / 2;
    int pillY = LY_RANGE - 4;
    int pillH = (dotY + dotR + 4) - pillY;
    gfx->fillRoundRect(pillX, pillY, pillW, pillH, 8, C_BLACK);

    Font_DrawCentered(text, LCD_WIDTH / 2, LY_RANGE, C_YELLOW, 2);
  } else {
    gfx->fillRoundRect(startX - dotR - 4, dotY - dotR - 3, totalW + 2 * (dotR + 4), 2 * dotR + 6, 6, C_BLACK);
  }

  for (int i = 0; i < totalCount; i++) {
    int x = startX + i * dotGap;
    if (i == activeIdx) {
      gfx->fillCircle(x, dotY, dotR, C_YELLOW);
    } else {
      gfx->drawCircle(x, dotY, dotR, C_GRAY);
    }
  }

  UI_DrawZoomControls();
}

void UI_DrawHomeMarker(int x, int y) {
  gfx->drawCircle(x, y, 8, C_CYAN);
  gfx->drawCircle(x, y, 4, C_YELLOW);
  gfx->fillCircle(x, y, 2, C_WHITE);
  gfx->drawFastHLine(x - 12, y, 24, C_DKGRAY);
  gfx->drawFastVLine(x, y - 12, 24, C_DKGRAY);
}

void UI_DrawCompassRose(int cx, int cy, int radius, float topHeadingDeg) {
  // Faint outer perimeter track
  gfx->drawCircle(cx, cy, radius, RGB565(24, 52, 70));
  gfx->drawCircle(cx, cy, radius - 1, RGB565(14, 30, 42));

  static const char* const LBL_EN[4] = { "N", "E", "S", "W" };
  static const char* const LBL_CZ[4] = { "S", "V", "J", "Z" };
  const char* const* card = (Lang_Get() == LANG_EN) ? LBL_EN : LBL_CZ;

  // Draw 10-degree and 30-degree aviation ticks
  for (int deg = 0; deg < 360; deg += 10) {
    float a = (deg - topHeadingDeg) * 0.0174532925f;
    float sinA = sinf(a);
    float cosA = cosf(a);

    bool isMajor30 = (deg % 30 == 0);
    int tickLen = isMajor30 ? 7 : 4;
    uint16_t tickCol = isMajor30 ? RGB565(80, 150, 185) : RGB565(32, 68, 88);

    if (deg == 0) {
      tickLen = 9;
      tickCol = C_YELLOW;
    }

    int x1 = cx + (int)roundf(radius * sinA);
    int y1 = cy - (int)roundf(radius * cosA);
    int x0 = cx + (int)roundf((radius - tickLen) * sinA);
    int y0 = cy - (int)roundf((radius - tickLen) * cosA);
    gfx->drawLine(x0, y0, x1, y1, tickCol);

    // Major 30-degree labels
    if (isMajor30) {
      int rLbl = radius - 12;
      int lx = cx + (int)roundf(rLbl * sinA);
      int ly = cy - (int)roundf(rLbl * cosA);

      if (deg == 0) {
        gfx->fillTriangle(lx, ly - 5, lx - 3, ly - 1, lx + 3, ly - 1, C_YELLOW);
        UI_Text(card[0], lx - 3, ly + 1, C_YELLOW, 1);
      } else if (deg == 90) {
        UI_Text(card[1], lx - 3, ly - 4, C_CYAN, 1);
      } else if (deg == 180) {
        UI_Text(card[2], lx - 3, ly - 4, C_CYAN, 1);
      } else if (deg == 270) {
        UI_Text(card[3], lx - 3, ly - 4, C_CYAN, 1);
      } else {
        // Standard aviation heading: 03, 06, 12, 15, 21, 24, 30, 33
        char hbuf[6];
        snprintf(hbuf, sizeof(hbuf), "%02d", deg / 10);
        int tw = Layout_TextW(hbuf, 1);
        UI_Text(hbuf, lx - tw / 2, ly - 4, RGB565(65, 115, 145), 1);
      }
    }
  }
}

static float calcGeoDistKm(double lat1, double lon1, double lat2, double lon2) {
  const float R = 6371.0f, D = 0.017453293f;
  float dLat = (float)(lat2 - lat1) * D;
  float dLon = (float)(lon2 - lon1) * D;
  float a = sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
            cosf((float)lat1 * D) * cosf((float)lat2 * D) * sinf(dLon * 0.5f) * sinf(dLon * 0.5f);
  if (a < 0) a = 0; else if (a > 1) a = 1;
  return 2.0f * R * asinf(sqrtf(a));
}

static bool s_photoFullscreen = false;

bool UI_IsPhotoFullscreen() { return s_photoFullscreen; }
static uint16_t* s_fsPhotoBuf = nullptr;

void UI_FreeFullscreenPhotoBuffer() {
  if (s_fsPhotoBuf) {
    heap_caps_free(s_fsPhotoBuf);
    s_fsPhotoBuf = nullptr;
  }
}

void UI_SetPhotoFullscreen(bool en) {
  s_photoFullscreen = en;
  if (!en) {
    UI_FreeFullscreenPhotoBuffer();
  }
}

static void drawScaledPhoto(const uint16_t* pixels, int iw, int ih, int targetX, int targetY, int tw, int th) {
  if (!pixels || iw <= 0 || ih <= 0 || tw <= 0 || th <= 0) return;
  for (int y = 0; y < th; y++) {
    int srcY = (y * ih) / th;
    if (srcY >= ih) srcY = ih - 1;
    const uint16_t* srcRow = pixels + (int32_t)srcY * iw;
    int dstY = targetY + y;
    if (dstY < 0 || dstY >= LCD_HEIGHT) continue;

    for (int x = 0; x < tw; x++) {
      int srcX = (x * iw) / tw;
      if (srcX >= iw) srcX = iw - 1;
      int dstX = targetX + x;
      if (dstX >= 0 && dstX < LCD_WIDTH) {
        gfx->drawPixel(dstX, dstY, srcRow[srcX]);
      }
    }
  }
}

static void drawScaledPhotoToBuffer(const uint16_t* pixels, int iw, int ih, int targetY, int tw, int th) {
  if (!pixels || iw <= 0 || ih <= 0 || tw <= 0 || th <= 0) return;
  if (!s_fsPhotoBuf) {
    s_fsPhotoBuf = (uint16_t*)heap_caps_malloc(480 * 300 * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (!s_fsPhotoBuf) return;

  const int cx = LCD_WIDTH / 2;
  const int cy = LCD_HEIGHT / 2;
  const int r2 = 239 * 239;

  for (int y = 0; y < th; y++) {
    int dstY = targetY + y;
    int dy = dstY - cy;
    int dy2 = dy * dy;

    int srcY = (y * ih) / th;
    if (srcY >= ih) srcY = ih - 1;
    const uint16_t* srcRow = pixels + (int32_t)srcY * iw;
    uint16_t* dstRow = s_fsPhotoBuf + y * tw;

    for (int x = 0; x < tw; x++) {
      int dx = x - cx;
      if (dx * dx + dy2 > r2) {
        dstRow[x] = 0x0000;
      } else {
        int srcX = (x * iw) / tw;
        if (srcX >= iw) srcX = iw - 1;
        dstRow[x] = srcRow[srcX];
      }
    }
  }

  gfx->draw16bitRGBBitmap(0, targetY, s_fsPhotoBuf, tw, th);
}

static void UI_DrawAircraftPhotoFullscreen(const Aircraft& ac, const RouteInfo* rt) {
  int iw = 0, ih = 0;
  const uint16_t* pixels = PlanePhoto_GetRgb565(&iw, &ih);
  if (!pixels || iw <= 0 || ih <= 0) {
    s_photoFullscreen = false;
    return;
  }

  // Clear background with deep dark avionics cockpit palette
  gfx->fillScreen(0x0000);

  // Aspect Fit horizontally to full 480px screen width so the ENTIRE aircraft (nose to tail) is 100% visible
  int tw = LCD_WIDTH;
  int th = (ih * tw) / iw;
  if (th > 300) th = 300; // clamp height so top and bottom avionics HUD have ample margin
  int targetX = 0;
  int targetY = (LCD_HEIGHT - th) / 2;

  // Render photo clipped cleanly to circular screen bounds as a single RGB565 bitmap
  drawScaledPhotoToBuffer(pixels, iw, ih, targetY, tw, th);

  // Outer circular avionics instrument ring
  gfx->drawCircle(LCD_WIDTH / 2, LCD_HEIGHT / 2, 238, ac.isMilitary ? C_RED : C_CYAN);

  // --- TOP AVIONICS HUD ---
  // Callsign (Size 2, bold)
  const char* cs = ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?");
  UI_TextCentered(cs, 22, ac.isMilitary ? C_RED : C_YELLOW, 2);

  // Full Aircraft Model & Registration (Size 1)
  char typeBuf[64] = "";
  char identified[48] = "";
  Aircraft_IdentifyModel(ac, identified, sizeof(identified));
  if (identified[0] && ac.reg[0]) {
    snprintf(typeBuf, sizeof(typeBuf), "%s [%s]", identified, ac.reg);
  } else if (ac.reg[0]) {
    snprintf(typeBuf, sizeof(typeBuf), "%s [%s]", ac.type[0] ? ac.type : "", ac.reg);
  } else if (identified[0]) {
    snprintf(typeBuf, sizeof(typeBuf), "%s", identified);
  } else if (ac.type[0]) {
    snprintf(typeBuf, sizeof(typeBuf), "%s", ac.type);
  }
  if (typeBuf[0]) {
    UI_TextCentered(typeBuf, 44, C_WHITE, 1);
  }

  // Flight Route (if available)
  if (rt && (rt->from[0] || rt->to[0])) {
    char routeBuf[64];
    snprintf(routeBuf, sizeof(routeBuf), "%s  ->  %s", rt->from[0] ? rt->from : "?", rt->to[0] ? rt->to : "?");
    UI_TextCentered(routeBuf, 60, 0x56E0, 1);
  }

  // --- BOTTOM AVIONICS HUD ---
  // Telemetry Bar
  const bool metric = Settings_MetricUnits();
  char tele[64];
  float altVal = metric ? (ac.altFt * 0.3048f) : ac.altFt;
  const char* altUnit = metric ? "m" : "ft";
  float spdVal = metric ? (ac.gsKt * 1.852f) : ac.gsKt;
  const char* spdUnit = metric ? "km/h" : "kt";

  if (ac.hasTrack) {
    snprintf(tele, sizeof(tele), "ALT %.0f %s   SPD %.0f %s   HDG %03.0f",
             altVal, altUnit, spdVal, spdUnit, ac.track);
  } else {
    snprintf(tele, sizeof(tele), "ALT %.0f %s   SPD %.0f %s",
             altVal, altUnit, spdVal, spdUnit);
  }

  int twTele = Layout_TextW(tele, 1);
  int bx = (LCD_WIDTH - twTele - 16) / 2;
  gfx->fillRoundRect(bx, 396, twTele + 16, 20, 5, 0x10A2);
  gfx->drawRoundRect(bx, 396, twTele + 16, 20, 5, 0x2124);
  UI_TextCenteredIn(tele, (LCD_WIDTH - twTele) / 2, twTele, 404, C_CYAN, 1);

  // Photographer attribution
  const char* photog = PlanePhoto_GetPhotographer();
  char credit[64];
  snprintf(credit, sizeof(credit), "Foto: (C) %s (Planespotters.net)", (photog && photog[0]) ? photog : "Planespotters.net");
  UI_TextCentered(credit, 428, C_GRAY, 1);

  // Return hint
  UI_TextCentered(T(S_TAP_TO_RETURN), 448, C_LTGRAY, 1);
}

void UI_DrawAircraftDetail(const Aircraft& ac, const RouteInfo* rt, int routeState, bool signalLost) {
  if (s_photoFullscreen) {
    UI_DrawAircraftPhotoFullscreen(ac, rt);
    return;
  }

  const bool metric = Settings_MetricUnits();
  const int photoW = 200, photoH = 133;
  const int photoX = (LCD_WIDTH - photoW) / 2;
  const int photoY = 50;

  gfx->fillRoundRect(photoX - 2, photoY - 2, photoW + 4, photoH + 4, 8, 0x0821);
  gfx->drawRoundRect(photoX - 2, photoY - 2, photoW + 4, photoH + 4, 8, ac.isMilitary ? C_RED : C_CYAN);

  PhotoState pState = PlanePhoto_GetState();
  if (pState == PHOTO_OK) {
    int iw = 0, ih = 0;
    const uint16_t* pixels = PlanePhoto_GetRgb565(&iw, &ih);
    if (pixels && iw > 0 && ih > 0) {
      float aspect = (float)iw / (float)ih;
      int tw = photoW;
      int th = (int)(tw / aspect);
      if (th > photoH) {
        th = photoH;
        tw = (int)(th * aspect);
      }
      int dx = photoX + (photoW - tw) / 2;
      int dy = photoY + (photoH - th) / 2;
      drawScaledPhoto(pixels, iw, ih, dx, dy, tw, th);
      gfx->fillRoundRect(photoX + photoW - 24, photoY + photoH - 17, 22, 14, 3, 0x18C3);
      UI_TextCenteredBox("+", photoX + photoW - 24, photoY + photoH - 17, 22, 14, C_WHITE, 1);
    }
    const char* photog = PlanePhoto_GetPhotographer();
    char credit[48];
    snprintf(credit, sizeof(credit), "Foto: %s", (photog && photog[0]) ? photog : "Planespotters.net");
    UI_TextCentered(credit, photoY + photoH + 4, C_GRAY, 1);
  } else if (pState == PHOTO_WAIT) {
    AircraftIconType iconType = Aircraft_GetIconType(ac);
    uint16_t sCol = ac.isMilitary ? C_RED : 0x07FF;
    Aircraft_DrawDetailedSilhouette(gfx, photoX + photoW / 2, photoY + photoH / 2 - 8, photoW - 20, photoH - 20, sCol, iconType);
    UI_TextCentered(T(S_PHOTO_WAIT), photoY + photoH - 18, C_GRAY, 1);
  } else if (pState == PHOTO_NONE) {
    AircraftIconType iconType = Aircraft_GetIconType(ac);
    uint16_t sCol = ac.isMilitary ? C_RED : 0x07FF;
    Aircraft_DrawDetailedSilhouette(gfx, photoX + photoW / 2, photoY + photoH / 2 - 8, photoW - 20, photoH - 20, sCol, iconType);
    const char* catName = Aircraft_GetCategoryName(iconType);
    UI_TextCentered(catName, photoY + photoH - 18, 0x52AA, 1);
  }

  const int cw = 310, ch = 126;
  const int cx = (LCD_WIDTH - cw) / 2;
  const int cy = 202;

  gfx->fillRoundRect(cx, cy, cw, ch, 12, 0x0821);
  gfx->drawRoundRect(cx, cy, cw, ch, 12, ac.isMilitary ? C_RED : 0x2FE6);

  const int bx = cx + cw - 16, by = cy + 14;
  gfx->fillCircle(bx, by, 9, 0x3000);
  gfx->drawCircle(bx, by, 9, C_GRAY);
  gfx->drawLine(bx - 3, by - 3, bx + 3, by + 3, C_WHITE);
  gfx->drawLine(bx - 3, by + 3, bx + 3, by - 3, C_WHITE);

  int ty = cy + 10;
  const char* cs = ac.callsign[0] ? ac.callsign : (ac.hex[0] ? ac.hex : "?");
  UI_Text(cs, cx + 14, ty, ac.isMilitary ? C_RED : C_YELLOW, 2);
  int csw = Layout_TextW(cs, 2);
  if (ac.isMilitary) {
    UI_Text("MIL", cx + 14 + csw + 8, ty + 2, C_RED, 1);
    csw += 28;
  }
  if (ac.type[0]) {
    UI_Text(ac.type, cx + 14 + csw + 10, ty, C_WHITE, 2);
  }
  ty += 23;

  const int col1X = cx + 14;
  const int col2X = cx + 160;

  char altStr[32];
  if (metric) snprintf(altStr, sizeof(altStr), "ALT %.0f m", ac.altFt * 0.3048f);
  else        snprintf(altStr, sizeof(altStr), "ALT %.0f ft", ac.altFt);
  UI_Text(altStr, col1X, ty, C_WHITE, 1);

  char vsStr[32];
  uint16_t vsCol = C_GRAY;
  if (ac.baroRate > 100.0f)       vsCol = C_GREEN;
  else if (ac.baroRate < -100.0f) vsCol = C_RED;
  if (metric) snprintf(vsStr, sizeof(vsStr), "V/S %+.1f m/s", ac.baroRate * 0.00508f);
  else        snprintf(vsStr, sizeof(vsStr), "V/S %+.0f", ac.baroRate);
  UI_Text(vsStr, col2X, ty, vsCol, 1);
  ty += 18;

  char spdStr[32];
  if (metric) snprintf(spdStr, sizeof(spdStr), "SPD %.0f km/h", ac.gsKt * 1.852f);
  else        snprintf(spdStr, sizeof(spdStr), "SPD %.0f kt", ac.gsKt);
  UI_Text(spdStr, col1X, ty, C_WHITE, 1);

  char hdgStr[32];
  if (ac.hasTrack) snprintf(hdgStr, sizeof(hdgStr), "HDG %03.0f", ac.track);
  else             snprintf(hdgStr, sizeof(hdgStr), "HDG ---");
  UI_Text(hdgStr, col2X, ty, C_WHITE, 1);
  ty += 18;

  float distKm = calcGeoDistKm(Settings_Lat(), Settings_Lon(), ac.lat, ac.lon);
  char distStr[32];
  if (metric) snprintf(distStr, sizeof(distStr), "DIST %.1f km", distKm);
  else        snprintf(distStr, sizeof(distStr), "DIST %.1f NM", distKm * 0.539957f);
  UI_Text(distStr, col1X, ty, C_WHITE, 1);

  char sqkStr[32];
  if (ac.squawk[0]) snprintf(sqkStr, sizeof(sqkStr), "SQK %s", ac.squawk);
  else              snprintf(sqkStr, sizeof(sqkStr), "SQK ----");
  UI_Text(sqkStr, col2X, ty, C_WHITE, 1);
  ty += 20;

  const int availW = cw - 28;
  if (routeState == ROUTE_WAIT) {
    UI_Text(T(S_ROUTE_WAIT), col1X, ty, C_GRAY, 1);
  } else if (rt && (rt->from[0] || rt->to[0])) {
    char rtLine[64];
    snprintf(rtLine, sizeof(rtLine), "%s -> %s", rt->from[0] ? rt->from : "?", rt->to[0] ? rt->to : "?");
    int rw = Layout_TextW(rtLine, 2);
    uint8_t fSize = (rw <= availW) ? 2 : 1;
    if (fSize == 1 && Layout_TextW(rtLine, 1) > availW) {
      int maxCh = availW / 6;
      if ((int)strlen(rtLine) > maxCh) { rtLine[maxCh - 2] = '.'; rtLine[maxCh - 1] = '.'; rtLine[maxCh] = '\0'; }
    }
    UI_Text(rtLine, col1X, ty, 0x56E0, fSize);
  } else {
    char typeLine[64] = "";
    char identified[48] = "";
    Aircraft_IdentifyModel(ac, identified, sizeof(identified));
    if (identified[0] && ac.reg[0])     snprintf(typeLine, sizeof(typeLine), "%s [%s]", identified, ac.reg);
    else if (ac.reg[0])                 snprintf(typeLine, sizeof(typeLine), "Reg: %s", ac.reg);
    else if (identified[0])             snprintf(typeLine, sizeof(typeLine), "%s", identified);
    if (typeLine[0]) {
      if (Layout_TextW(typeLine, 1) > availW) {
        int maxCh = availW / 6;
        if ((int)strlen(typeLine) > maxCh) { typeLine[maxCh - 2] = '.'; typeLine[maxCh - 1] = '.'; typeLine[maxCh] = '\0'; }
      }
      UI_Text(typeLine, col1X, ty, 0xFDE0, 1);
    }
  }

  if (signalLost) {
    const char* lost = T(S_SIGNAL_LOST);
    UI_Text(lost, cx + cw - 14 - Layout_TextW(lost, 1), cy + ch - 12, C_YELLOW, 1);
  }
}

void UI_DrawOtaProgress(const char* sourceName, int percent, size_t bytesWritten, size_t totalBytes, const char* statusMsg) {}
void UI_DrawOtaWritingStaticScreen(const char* sourceName, const char* customMsg) {}

void UI_DrawCompassWidget(int cx, int cy, int r, float headingDeg, uint16_t primaryCol, bool showCard) {
  if (!gfx) return;
  gfx->fillCircle(cx, cy, r, 0x0821);
  gfx->drawCircle(cx, cy, r, primaryCol);

  gfx->drawLine(cx, cy - r, cx, cy - r + 3, primaryCol);
  gfx->drawLine(cx + r, cy, cx + r - 3, cy, primaryCol);
  gfx->drawLine(cx, cy + r, cx, cy + r - 3, primaryCol);
  gfx->drawLine(cx - r, cy, cx - r + 3, cy, primaryCol);

  float a = headingDeg * 0.0174532925f;
  float sa = sinf(a), ca = cosf(a);

  int nx = cx + (int)((r - 3) * sa);
  int ny = cy - (int)((r - 3) * ca);
  int sx = cx - (int)((r - 3) * sa);
  int sy = cy + (int)((r - 3) * ca);

  int px = (int)(2.8f * ca);
  int py = (int)(2.8f * sa);

  gfx->fillTriangle(cx + px, cy + py, cx - px, cy - py, nx, ny, C_RED);
  gfx->fillTriangle(cx + px, cy + py, cx - px, cy - py, sx, sy, C_WHITE);
  gfx->fillCircle(cx, cy, 2, C_WHITE);

  if (showCard) {
    char hbuf[16];
    int degInt = ((int)roundf(headingDeg) % 360 + 360) % 360;
    snprintf(hbuf, sizeof(hbuf), "%03d", degInt);
    int hw = Layout_TextW(hbuf, 1);
    int bw = hw + 8;
    int bh = 12;
    int bx = cx - hw / 2 - 4;
    int by = cy + r + 3;
    gfx->fillRoundRect(bx, by, bw, bh, 3, 0x0821);
    gfx->drawRoundRect(bx, by, bw, bh, 3, 0x2124);
    UI_TextCenteredBox(hbuf, bx, by, bw, bh, C_CYAN, 1);
  }
}


