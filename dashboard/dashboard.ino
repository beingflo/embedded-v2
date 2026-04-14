#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "EPD.h"

const char* ssid     = "";
const char* password = "";
const char* authToken = "";

#define IMAGE_URL "https://push.events.marending.dev/api/dashboard"
#define IMG_W 792
#define IMG_H 272
#define IMG_ROW_BYTES ((IMG_W + 7) / 8)  // 99
#define IMG_SIZE (IMG_ROW_BYTES * IMG_H)  // 26928
#define DEFAULT_SLEEP_SECONDS 120

uint8_t ImageBW[27200];
int sleepSeconds = DEFAULT_SLEEP_SECONDS;

uint8_t* fetchImage() {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, IMAGE_URL);
    http.addHeader("Authorization", authToken);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    const char* headerKeys[] = {"X-Refresh-Seconds"};
    http.collectHeaders(headerKeys, 1);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        return nullptr;
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
        return nullptr;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t bufLen = 0;
    size_t target = (contentLength > 0) ? contentLength : IMG_SIZE;
    unsigned long lastDataTime = millis();
    while (bufLen < target) {
        if (stream->available()) {
            size_t chunk = stream->readBytes(buf + bufLen, target - bufLen);
            bufLen += chunk;
            lastDataTime = millis();
        } else if (millis() - lastDataTime > 10000) {
            Serial.println("Download timed out");
            free(buf);
            http.end();
            return nullptr;
        }
        delay(1);
    }
    http.end();
    Serial.printf("Downloaded %d bytes\n", bufLen);

    return buf;
}

void setup() {
    Serial.begin(115200);

    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH);
    EPD_GPIOInit();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
}

void loop() {
    // Connect WiFi and fetch image
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
        delay(500);
        Serial.print(".");
        wifiAttempts++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(" failed, retrying next cycle");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay((uint32_t)sleepSeconds * 1000UL);
        return;
    }
    Serial.println(" connected");

    uint8_t* buf = fetchImage();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // Clear and display
    Paint_Clear(WHITE);
    EPD_FastMode1Init();
    EPD_Display_Clear();
    EPD_Update();

    if (buf) {
        EPD_ShowPicture(0, 0, IMG_W, IMG_H, buf, BLACK);
        free(buf);
    }

    EPD_Display(ImageBW);
    EPD_FastUpdate();

    Serial.printf("Sleeping for %d seconds...\n", sleepSeconds);
    Serial.flush();
    delay((uint32_t)sleepSeconds * 1000UL);
}
