#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;

void setup()
{
  Serial.begin(115200);

  while (!Serial)
    delay(100);

  unsigned status = bmp.begin(BMP280_ADDRESS_ALT);

  while (!status)
  {
    Serial.println("Not connected");
    delay(1000);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X8,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void loop()
{
  float temperature = bmp.readTemperature();
  Serial.print(F("Temperature = "));
  Serial.print(temperature);
  Serial.println(" *C");

  float abs_pressure = bmp.readPressure();
  float altitude = 412;
  float pressure = pow((1 - ((0.0065 * altitude) / (temperature + 273.15 + 0.0065 * altitude))), -5.257) * abs_pressure / 100;
  Serial.print(F("Pressure = "));
  Serial.print(pressure);
  Serial.println(" hPa");

  delay(2000);
}