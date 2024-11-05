#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <esp_now.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include "DHT20.h"

DHT20 DHT;

SCD30 airSensor;

RTC_DATA_ATTR int counter = 0;

const char *ssid = "";
const char *password = "";
String endpoint_88 = "";
String endpoint_e0 = "";
String endpoint_fc = "";

void connectSensor()
{
  Wire.begin();

  while (airSensor.begin() == false)
  {
    Serial.println("Sensor not detected!");
    delay(1000);
  }
  airSensor.setTemperatureOffset(1.8);
  DHT.begin();  

  delay(1000);
}

void setup()
{
  Serial.begin(921600);

  connectSensor();
}

void loop()
{
  uint16_t co2 = 0;
  float temp = 0.0;
  float hum = 0.0;

  if (airSensor.dataAvailable()) {
    co2 = airSensor.getCO2();
    temp = airSensor.getTemperature();
    hum = airSensor.getHumidity();

    Serial.print("co2(ppm):");
    Serial.print(co2);

    Serial.print(" temp(C):");
    Serial.print(temp);

    Serial.print(" humidity(%):");
    Serial.print(hum);

    Serial.println();

    DHT.read();
    Serial.print("DHT: temp(C):");
    Serial.print(DHT.getTemperature());
    Serial.print(" humidity(%):");
    Serial.print(DHT.getHumidity());

    Serial.println();
  } else {
    Serial.println("Waiting for new data");
  }

  delay(1000);
}
