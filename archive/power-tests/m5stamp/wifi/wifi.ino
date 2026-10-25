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

void setup()
{
  pinMode(A5, OUTPUT);
  digitalWrite(A5, HIGH);
  Serial.begin(115200);
  initWiFi();
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
}

void loop()
{
  digitalWrite(A5, LOW);
  // initWiFi();
}
