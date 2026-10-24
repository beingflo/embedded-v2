#include <Arduino.h>

int led = D10;

void setup()
{
  pinMode(led, OUTPUT);
}

void loop()
{
  digitalWrite(led, HIGH);
  gpio_hold_en(GPIO_NUM_10);
  esp_sleep_enable_timer_wakeup(10000000);
  esp_light_sleep_start();
  gpio_hold_dis(GPIO_NUM_10);
  digitalWrite(led, LOW);
  esp_sleep_enable_timer_wakeup(10000000);
  esp_light_sleep_start();
}
