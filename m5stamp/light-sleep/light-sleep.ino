#include <Arduino.h>

void setup()
{
  pinMode(A5, OUTPUT);
}

void loop()
{
  digitalWrite(A5, HIGH);
  gpio_hold_en(GPIO_NUM_5);
  esp_sleep_enable_timer_wakeup(10000000);
  esp_light_sleep_start();
  gpio_hold_dis(GPIO_NUM_5);
  digitalWrite(A5, LOW);
  esp_sleep_enable_timer_wakeup(10000000);
  esp_light_sleep_start();
}
