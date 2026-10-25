#include "Arduino.h"
#include <WiFi.h>

const char *ssid = "";
const char *password = "";

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

double duration = 0;

void setup()
{
  unsigned long timeBegin = micros();
  Serial.begin(115200);
  initWiFi();

  unsigned long timeEnd = micros();
  unsigned long duration_long = timeEnd - timeBegin;
  duration = (double)duration_long / 1000.0;

  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  Serial.printf("Duration (ms): %f\n", duration);
}

void loop()
{
  delay(1000);
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  // initWiFi();
}
