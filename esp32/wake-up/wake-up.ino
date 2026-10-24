#include "Arduino.h"

RTC_DATA_ATTR int value = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println(value);
  value += 1;

  esp_sleep_enable_timer_wakeup(1);

  esp_deep_sleep_start();
}

void loop()
{
  // This is not going to be called
}
