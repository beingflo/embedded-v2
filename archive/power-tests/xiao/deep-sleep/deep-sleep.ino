#include "Arduino.h"

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 10

RTC_DATA_ATTR int value = 0;

void setup()
{
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop()
{
  // This is not going to be called
}
