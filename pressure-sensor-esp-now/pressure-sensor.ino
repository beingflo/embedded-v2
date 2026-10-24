#include <Adafruit_BMP280.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t receiver[] = {0x08, 0x3A, 0xF2, 0x14, 0x2A, 0x88};

Adafruit_BMP280 bmp;

esp_now_peer_info_t peerInfo;

int success = 0;

struct msg_data
{
  float pressure;
  float temperature;
  int num_tries;
};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

  if (status == ESP_NOW_SEND_SUCCESS)
  {
    success = 1;
  }
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  unsigned status = bmp.begin(BMP280_ADDRESS_ALT);

  while (!status)
  {
    Serial.println("Not connected");
    delay(1000);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X8,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  if (esp_now_register_send_cb(OnDataSent) != ESP_OK)
  {
    Serial.println("Error registering callback");
    return;
  }

  memcpy(peerInfo.peer_addr, receiver, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Error adding peer");
    return;
  }
}

void loop()
{
  esp_wifi_start();
  WiFi.mode(WIFI_STA);

  float temperature = 0;
  float pressure = 0;

  if (bmp.takeForcedMeasurement())
  {
    temperature = bmp.readTemperature();
    Serial.print(F("Temperature = "));
    Serial.print(temperature);
    Serial.println(" *C");

    float abs_pressure = bmp.readPressure();
    float altitude = 412;
    // Convert from absolute pressure to sea level pressure
    pressure = pow((1 - ((0.0065 * altitude) / (temperature + 273.15 + 0.0065 * altitude))), -5.257) * abs_pressure / 100;
    Serial.print(F("Pressure = "));
    Serial.print(pressure);
    Serial.println(" hPa");
  }

  int num_tries = 0;

  // Resend until data was received by recipient
  while (!success && num_tries < 5)
  {
    esp_err_t result = ESP_FAIL;

    num_tries += 1;

    int send_tries = 0;
    // Try sending until successfully sent
    while (result != ESP_OK && send_tries < 20)
    {
      send_tries += 1;

      struct msg_data data = {pressure, temperature, num_tries};

      result = esp_now_send(receiver, (uint8_t *)&data, sizeof(msg_data));

      if (result != ESP_OK)
      {
        Serial.println("Error sending the data");
        break;
      }
    }

    if (send_tries == 20)
    {
      Serial.println("ESP can't send");
    }

    // Allow time for callback to signal success
    delay(100);
  }

  if (num_tries == 5)
  {
    Serial.println("Retries exhausted");
  }

  success = 0;

  esp_wifi_stop();
  esp_sleep_enable_timer_wakeup(600000000);

  esp_deep_sleep_start();
}