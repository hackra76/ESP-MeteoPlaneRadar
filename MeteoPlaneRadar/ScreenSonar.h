#pragma once
#include <Arduino.h>
#include <lvgl.h>

void ScreenSonar_Init(lv_obj_t* parent);
void ScreenSonar_Enter();
void ScreenSonar_Draw();
void ScreenSonar_ChangeRange(int dir);
void ScreenSonar_RangeText(char* out, size_t cap);
