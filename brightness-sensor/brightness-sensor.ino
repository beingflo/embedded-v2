// Living-room brightness and barometric pressure from a BH1750 and a BMP280.
//
// One row every 30 s as the source `brightness-barometer-living-room`, with the
// same field names and units the migrated history uses: `absolute_pressure` in
// Pa as the sensor reports it, `pressure` reduced to sea level in hPa.
//
// Credentials and the endpoint live in ../common/config.h, the upload path in
// ../common/acta.h.

#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <Wire.h>

#include "../common/acta.h"

#define SOURCE "brightness-barometer-living-room"

// Height above sea level, used to reduce the absolute pressure to sea level.
#define ALTITUDE_M 430.0

BH1750 light;
Adafruit_BMP280 bmp;

void connectSensor() {
  Wire.begin();

  light.begin(BH1750::ONE_TIME_HIGH_RES_MODE);

  while (!bmp.begin(BMP280_ADDRESS_ALT)) {
    Serial.println("BMP not connected ...");
    delay(1000);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X8,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  delay(1000);
}

void setup() {
  Serial.begin(921600);

  connectSensor();
  connectWifi();
}

void loop() {
  if (light.measurementReady(true) && bmp.takeForcedMeasurement()) {
    float lux = light.readLightLevel();
    float temperature = bmp.readTemperature();
    float absolute_pressure = bmp.readPressure();

    // Reduce to sea level, the formula the previous system used, so the series
    // stays comparable across the move to acta.
    float pressure = pow((1 - ((0.0065 * ALTITUDE_M) / (temperature + 273.15 + 0.0065 * ALTITUDE_M))), -5.257) * absolute_pressure / 100;

    Serial.printf("lux:%.2f temp:%.2f pressure:%.2f absolute_pressure:%.2f\n",
                  lux, temperature, pressure, absolute_pressure);

    // A failed read can come back as NaN, which is not JSON: acta would reject
    // the whole request with 400 instead of just this sample going missing.
    if (!isfinite(lux) || !isfinite(temperature) || !isfinite(absolute_pressure) || !isfinite(pressure)) {
      Serial.println("implausible reading, skipped");
    } else {
      String payload = String("{\"lux\":") + String(lux, 2) +
                       ",\"temperature\":" + String(temperature, 2) +
                       ",\"absolute_pressure\":" + String(absolute_pressure, 2) +
                       ",\"pressure\":" + String(pressure, 2) + "}";
      actaPost(SOURCE, payload);
    }
  } else {
    Serial.printf("BH1750 or BMP280 measurement not ready\n");
  }

  delay(29500);
  light.configure(BH1750::ONE_TIME_HIGH_RES_MODE);
  delay(500);
}
