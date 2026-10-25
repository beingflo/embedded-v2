// Living-room CO₂, temperature and humidity from an SCD30.
//
// One row every 30 s as the source `co2-sensor-living-room`, which is also the
// series acta's `/health` probe watches: if this device stops reporting for ten
// minutes, acta reports itself unhealthy.
//
// Credentials and the endpoint live in ../common/config.h, the upload path in
// ../common/acta.h.

#include <Wire.h>

#include "SparkFun_SCD30_Arduino_Library.h"

#include "../common/acta.h"

#define SOURCE "co2-sensor-living-room"

SCD30 scd30;

void connectSensor() {
  Wire.begin();

  while (scd30.begin() == false) {
    Serial.println("Sensor not detected!");
    delay(1000);
  }
  scd30.setTemperatureOffset(1.8);

  delay(1000);
}

void setup() {
  Serial.begin(921600);

  connectSensor();
  connectWifi();
}

void loop() {
  if (scd30.dataAvailable()) {
    uint16_t co2 = scd30.getCO2();
    float temp = scd30.getTemperature();
    float hum = scd30.getHumidity();

    Serial.printf("co2:%u temp:%.2f humidity:%.2f\n", co2, temp, hum);

    // A failed read can come back as NaN, which is not JSON: acta would reject
    // the whole request with 400 instead of just this sample going missing.
    if (co2 == 0 || !isfinite(temp) || !isfinite(hum)) {
      Serial.println("implausible reading, skipped");
    } else {
      String payload = String("{\"co2\":") + co2 +
                       ",\"temperature\":" + String(temp, 2) +
                       ",\"humidity\":" + String(hum, 2) + "}";
      actaPost(SOURCE, payload);
    }
  } else {
    Serial.println("SCD30 data not ready");
  }

  delay(30000);
}
