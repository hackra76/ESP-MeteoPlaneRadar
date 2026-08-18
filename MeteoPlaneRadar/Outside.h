// =============================================================================
//  MeteoPlaneRadar
//  Clock + outside temperature - the one-line readout under the screen dots.
//
//  THE CLOCK DOES NOT USE NTP. One user's ISP blocks UDP port 123 outright, and
//  the whole thing is unnecessary anyway: every HTTPS response we already make
//  carries a "Date:" header in GMT, accurate to the second. We fetch aircraft
//  every 5-15 s, so the clock arrives with the first poll, costs no extra
//  connection, needs no extra port and cannot be firewalled off separately.
//  It is also re-seeded on every fetch, so drift never accumulates.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

// Feed the raw value of an HTTP "Date" header, e.g.
// "Sun, 09 Aug 2026 20:00:56 GMT". Anything unparseable is ignored.
// Called by ADSB.cpp and CHMU.cpp after every successful request.
void Outside_NoteHttpDate(const char* date);

bool Outside_TimeValid();

// The forecast module fetches the current temperature as part of its own
// request. When it does, it hands the value over here instead of letting this
// module make a second call for the same number.
void Outside_NoteTemp(float degC);

// Poll the outside temperature when it is due. Cheap no-op otherwise.
void Outside_Tick();

// The finished readout, e.g. "21:42  18 degC". Empty until something is known;
// shows only the half that IS known. Never longer than OUTSIDE_TEXT_MAX-1.
#define OUTSIDE_TEXT_MAX 20
void Outside_StatusText(char* buf, size_t cap);
