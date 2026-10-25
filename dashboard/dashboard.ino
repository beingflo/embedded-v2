// The epaper dashboard in the living room.
//
// The panel is a 792×272 one-bit display on an ESP32-S3. It holds no clock, no
// fonts and no chart library: once every couple of minutes it pulls one
// pre-rendered frame from acta (`GET /api/dashboard`, see
// `acta/service/src/display.rs`), copies it into the paint buffer and goes back
// to waiting. The picture is the living-room CO2 curve of the last six hours
// with four scalar readings beside it.
//
// Credentials and the endpoint live in ../common/config.h, the fetch path in
// ../common/acta.h. The credential is the restricted one, so the panel can read
// the home dashboard and nothing else.
//
// The refresh interval comes from the server (`X-Refresh-Seconds`), so it can be
// changed without opening the panel up to reflash it.

#include <Arduino.h>
#include <WiFi.h>

#include "EPD.h"

#include "../common/acta.h"

#if !defined(ACTA_DISPLAY_URL) || !defined(ACTA_HOME_USER) || !defined(ACTA_HOME_PASSWORD)
#error "common/config.h needs ACTA_DISPLAY_URL, ACTA_HOME_USER and ACTA_HOME_PASSWORD for the panel"
#endif

// The visible area of the panel. The controller's frame is 800 px wide, but the
// last 8 px are bezel.
#define IMG_W 792
#define IMG_H 272
#define IMG_SIZE (((IMG_W + 7) / 8) * IMG_H)  // 99 bytes per row, 26928 total

// Only used until the first response tells us what the server wants.
#define DEFAULT_REFRESH_SECONDS 120

// Paint buffer for the panel's controller, and the frame fetched from acta.
static uint8_t ImageBW[EPD_W * EPD_H / 8];
static uint8_t Frame[IMG_SIZE];

static long refreshSeconds = DEFAULT_REFRESH_SECONDS;

// Push the fetched frame onto the panel. Clearing first and updating twice is
// what keeps a partial repaint from ghosting the previous picture: the fast
// update path only flips the pixels it is given, and at ambient temperature it
// needs the clean pass to settle.
static void draw() {
    Paint_Clear(WHITE);
    EPD_FastMode1Init();
    EPD_Display_Clear();
    EPD_Update();

    EPD_ShowPicture(0, 0, IMG_W, IMG_H, Frame, BLACK);

    EPD_Display(ImageBW);
    EPD_FastUpdate();
}

void setup() {
    Serial.begin(921600);

    // Panel power, held high for as long as the board runs.
    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);

    EPD_GPIOInit();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
}

void loop() {
    const int got = actaFetch(ACTA_DISPLAY_URL, ACTA_HOME_USER, ACTA_HOME_PASSWORD, Frame,
                              sizeof(Frame), &refreshSeconds);

    // The radio is the largest load on an always-powered board; drop it while
    // the panel is drawing and idling.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    if (got == (int)sizeof(Frame)) {
        draw();
    } else {
        // Keep what is on the panel: a failed fetch should not blank the wall.
        Serial.println("[panel] no new picture, keeping the current one");
    }

    Serial.printf("[panel] next picture in %ld s\n", refreshSeconds);
    Serial.flush();
    delay((uint32_t)refreshSeconds * 1000UL);
}
