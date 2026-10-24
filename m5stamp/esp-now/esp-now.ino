#include <esp_now.h>
#include <WiFi.h>

uint8_t receiver[] = {0x08, 0x3A, 0xF2, 0x14, 0x2A, 0x88};

esp_now_peer_info_t peerInfo;

int counter = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
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

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, receiver, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop()
{
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiver, (uint8_t *)&counter, sizeof(counter));

  Serial.printf("Trying to print %d\n", counter);

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
  counter++;
  delay(2000);
}