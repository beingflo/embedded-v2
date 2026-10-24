#include <WiFi.h>
#include <HTTPClient.h>

RTC_DATA_ATTR int counter = 0;

const char *ssid = "SSID";
const char *password = "PASSWORD";

String endpoint = "ENDPOINT";

void setup()
{
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);

  WiFi.begin(ssid, password);

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print(".");
  }

  Serial.print("Connected!");
  Serial.println(WiFi.localIP());
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    String body = "#TYPE some_metric gauge\nsome_metric " + String(counter) + "\n";

    http.begin(endpoint.c_str());

    int responseCode = http.POST(body);

    if (responseCode > 0)
    {
      Serial.print("HTTP Response: ");
      Serial.println(responseCode);
    }
    else
    {
      Serial.print("Error: ");
      Serial.println(responseCode);
    }

    http.end();
  }

  counter += 1;

  esp_sleep_enable_timer_wakeup(1);
  Serial.println("Enter sleep");
  esp_deep_sleep_start();
}
