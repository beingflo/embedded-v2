#include <WiFi.h>

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  Serial.println(WiFi.macAddress());

  delay(2000);
}
