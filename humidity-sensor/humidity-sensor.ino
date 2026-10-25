// Laundry-room temperature and humidity from a DHT20.
//
// One row every 30 s as the source `humidity-laundry-room`.
//
// Credentials and the endpoint live in ../common/config.h, the upload path in
// ../common/acta.h.

#include <Wire.h>

#include "DHT20.h"

#include "../common/acta.h"

#define SOURCE "humidity-laundry-room"

DHT20 dht;

void connectSensor() {
  Wire.begin();

  dht.begin();

  delay(1000);
}

void setup() {
  Serial.begin(921600);

  connectSensor();
  connectWifi();
}

void loop() {
  int err = dht.read();
  if (err == DHT20_OK) {
    float temp = dht.getTemperature();
    float hum = dht.getHumidity();

    Serial.printf("temp:%.2f humidity:%.2f\n", temp, hum);

    // A failed read can come back as NaN, which is not JSON: acta would reject
    // the whole request with 400 instead of just this sample going missing.
    if (!isfinite(temp) || !isfinite(hum)) {
      Serial.println("implausible reading, skipped");
    } else {
      String payload = String("{\"temperature\":") + String(temp, 2) +
                       ",\"humidity\":" + String(hum, 2) + "}";
      actaPost(SOURCE, payload);
    }
  } else {
    Serial.printf("DHT read failure: %d\n", err);
  }

  delay(30000);
}
