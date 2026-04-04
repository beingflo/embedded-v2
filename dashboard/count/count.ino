#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <JPEGDEC.h>
#include "EPD.h"

const char* ssid     = "";
const char* password = "";

#define IMAGE_URL "https://picsum.photos/792/272?grayscale"

uint8_t ImageBW[27200];

JPEGDEC jpeg;

int jpegDraw(JPEGDRAW* pDraw) {
    uint8_t* pixels = (uint8_t*)pDraw->pPixels;
    int pixelW = pDraw->iWidth * 8;
    int pixelH = pDraw->iHeight * 8;
    int originX = pDraw->x * 8;
    int originY = pDraw->y * 8;
    for (int y = 0; y < pixelH; y++) {
        for (int x = 0; x < pixelW; x++) {
            uint8_t gray = pixels[y * pixelW + x];
            uint16_t color = (gray >= 128) ? WHITE : BLACK;
            Paint_SetPixel(originX + x, originY + y, color);
        }
    }
    return 1;
}

void fetchAndDisplay() {
    WiFiClientSecure client;
    client.setInsecure(); // skip certificate verification

    HTTPClient http;
    http.begin(client, IMAGE_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        return;
    }

    int contentLength = http.getSize();
    Serial.printf("Content-Length: %d bytes\n", contentLength);

    WiFiClient* stream = http.getStreamPtr();
    uint8_t* buf = nullptr;
    size_t bufLen = 0;

    if (contentLength > 0) {
        buf = (uint8_t*)malloc(contentLength);
        if (!buf) {
            Serial.println("malloc failed");
            http.end();
            return;
        }
        while (bufLen < (size_t)contentLength) {
            if (stream->available()) {
                size_t chunk = stream->readBytes(buf + bufLen, contentLength - bufLen);
                bufLen += chunk;
            }
            delay(1);
        }
    } else {
        // Chunked or unknown length — grow buffer as data arrives
        const size_t CHUNK = 4096;
        size_t capacity = CHUNK;
        buf = (uint8_t*)malloc(capacity);
        while (http.connected() || stream->available()) {
            size_t avail = stream->available();
            if (avail) {
                if (bufLen + avail > capacity) {
                    capacity = bufLen + avail + CHUNK;
                    buf = (uint8_t*)realloc(buf, capacity);
                    if (!buf) {
                        http.end();
                        return;
                    }
                }
                bufLen += stream->readBytes(buf + bufLen, avail);
            }
            delay(1);
        }
    }

    http.end();
    Serial.printf("Downloaded %d bytes\n", bufLen);

    if (jpeg.openRAM(buf, bufLen, jpegDraw)) {
        jpeg.setPixelType(EIGHT_BIT_GRAYSCALE);
        jpeg.decode(0, 0, 0);
        jpeg.close();
    } else {
        Serial.println("JPEG decode failed");
    }

    free(buf);
}

void setup() {
    Serial.begin(115200);

    pinMode(7, OUTPUT);
    digitalWrite(7, HIGH); // power on display

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
}

void loop() {}
