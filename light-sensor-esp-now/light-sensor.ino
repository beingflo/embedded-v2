#include <Wire.h>
#include <BH1750.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t receiver[] = {0x08, 0x3A, 0xF2, 0x14, 0x2A, 0x88};

esp_now_peer_info_t peerInfo;

BH1750 lightMeter;

int success = 0;

float lux = 0;
int moisture = 0;
float vBat = 0;

struct msg_data
{
  float lux;
  int moisture;
  float vBat;
  int num_tries;
};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    success = 1;
  }
}

void configure_esp_now()
{
  WiFi.mode(WIFI_STA);

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

void configure_sensors()
{
  Serial.println("Configuring sensors");
  Wire.begin();

  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
}

void take_measurements()
{
  Serial.println("Taking measurements");

  while (!lightMeter.measurementReady(true))
  {
    yield();
  }

  // Power moisture meter with GPIO to save power in deep sleep
  digitalWrite(D3, HIGH);
  delay(100);
  moisture = analogRead(A2);
  digitalWrite(D3, LOW);
  Serial.print("Moisture: ");
  Serial.println(moisture);

  lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.println(lux);

  lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE);
}

void transmit_data()
{
  esp_wifi_start();
  WiFi.mode(WIFI_STA);

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

      struct msg_data data = {lux, moisture, vBat, num_tries};

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
}

void read_battery_voltage()
{
  uint16_t raw;

  int num = 5;
  while (num > 0)
  {
    raw = analogRead(A0);
    num--;
  }

  float voltage = ((float)raw / 4095) * 3.1 * 2;

  Serial.println(voltage);

  vBat = voltage;
}

void setup()
{
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);

  pinMode(D3, OUTPUT);

  configure_esp_now();
  configure_sensors();
}

void loop()
{
  take_measurements();
  read_battery_voltage();

  transmit_data();

  esp_sleep_enable_timer_wakeup(30000000);
  esp_deep_sleep_start();
}