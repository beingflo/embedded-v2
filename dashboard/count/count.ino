#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "EPD.h"

const char* ssid     = "";
const char* password = "";

#define IMAGE_URL "https://push.events.marending.dev/api/dashboard"
#define IMG_W 792
#define IMG_H 272
#define IMG_ROW_BYTES ((IMG_W + 7) / 8)  // 99
#define IMG_SIZE (IMG_ROW_BYTES * IMG_H)  // 26928
#define DEFAULT_SLEEP_SECONDS 120

uint8_t ImageBW[27200];
int sleepSeconds = DEFAULT_SLEEP_SECONDS;

void fetchAndDisplay() {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, IMAGE_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    const char* headerKeys[] = {"X-Refresh-Seconds"};
    http.collectHeaders(headerKeys, 1);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        return;
    }

    String refreshHeader = http.header("X-Refresh-Seconds");
    if (refreshHeader.length() > 0) {
        sleepSeconds = refreshHeader.toInt();
    }

    int contentLength = http.getSize();
    Serial.printf("Content-Length: %d bytes\n", contentLength);

    uint8_t* buf = (uint8_t*)malloc(IMG_SIZE);
    if (!buf) {
        Serial.println("malloc failed");
        http.end();
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t bufLen = 0;
    size_t target = (contentLength > 0) ? contentLength : IMG_SIZE;
    while (bufLen < target) {
        if (stream->available()) {
            size_t chunk = stream->readBytes(buf + bufLen, target - bufLen);
            bufLen += chunk;
        }
        delay(1);
    }
    http.end();
    Serial.printf("Downloaded %d bytes\n", bufLen);

    EPD_ShowPicture(0, 0, IMG_W, IMG_H, buf, BLACK);
    free(buf);
}

void setup() {
    Serial.begin(115200);

    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);

    EPD_GPIOInit();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
    Paint_Clear(WHITE);
    EPD_FastMode1Init();

    EPD_Display_Clear();
    EPD_Update();
    EPD_Clear_R26A6H();

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" connected");

    fetchAndDisplay();

    EPD_Display(ImageBW);
    EPD_PartUpdate();
    EPD_DeepSleep();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    Serial.printf("Sleeping for %d seconds...\n", sleepSeconds);
    Serial.flush();
    esp_sleep_enable_timer_wakeup((uint64_t)sleepSeconds * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() {}
