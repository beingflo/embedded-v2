#include <esp_now.h>
#include <WiFi.h>

struct msg_data
{
  int counter;
  int num_tries;
};

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  struct msg_data data;
  memcpy(&data, incomingData, sizeof(msg_data));

  Serial.printf("%d %d\n", data.counter, data.num_tries);
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
}
