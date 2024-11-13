#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Wire.h>
#include <esp_now.h>
#include <BH1750.h>
#include <Adafruit_BMP280.h>

BH1750 light;
Adafruit_BMP280 bmp;

RTC_DATA_ATTR int counter = 0;

const char *ssid = "";
const char *password = "";
String observatory_url = "https://observatory.marending.dev/api/data";
String observatory_emitter = "";

// Expires 2035
const char *rootCA =  "-----BEGIN CERTIFICATE-----\n"
                      "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
                      "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
                      "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
                      "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
                      "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
                      "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
                      "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
                      "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
                      "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
                      "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
                      "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
                      "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
                      "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
                      "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
                      "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
                      "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
                      "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
                      "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
                      "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
                      "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
                      "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
                      "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
                      "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
                      "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
                      "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
                      "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
                      "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
                      "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
                      "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
                      "-----END CERTIFICATE-----\n";

void connectWifi() {
  WiFi.mode(WIFI_AP_STA);
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

void connectSensor() {
  Wire.begin();

  light.begin(BH1750::ONE_TIME_HIGH_RES_MODE);

  unsigned status = bmp.begin(BMP280_ADDRESS_ALT);

  while (!status) {
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

void sendData(float lux, float temperature, float absolute_pressure, float pressure) {
  if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Wifi not connected\n");
      return;
  }

  NetworkClientSecure *client = new NetworkClientSecure;

  if (client) {
    client->setCACert(rootCA);

    {
      HTTPClient https;

      Serial.print("[HTTPS] begin\n");
      if (https.begin(*client, observatory_url.c_str())) {  // HTTPS
        https.addHeader("emitter", observatory_emitter.c_str());
        https.addHeader("Content-Type", "application/json");

        String body = "{ \"bucket\": \"brightness-barometer-living-room\", \"payload\": { \"lux\": " + String(lux) + ", \"temperature\": " + String(temperature) + ", \"absolute_pressure\": " + String(absolute_pressure) + ", \"pressure\": " + String(pressure) + " } }";

        int httpCode = https.POST(body);

        Serial.printf("[HTTPS] response code: %d\n", httpCode);
        if (httpCode <= 0) {
          Serial.printf("[HTTPS] failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
    }

    delete client;
  } else {
    Serial.println("Unable to create client");
  }
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
    float abs_pressure = bmp.readPressure();
    float altitude = 430;
    // Convert from absolute pressure to sea level pressure
    float pressure = pow((1 - ((0.0065 * altitude) / (temperature + 273.15 + 0.0065 * altitude))), -5.257) * abs_pressure / 100;

    Serial.print("lux:");
    Serial.print(lux);
    Serial.println();

    Serial.print("temp:");
    Serial.print(temperature);
    Serial.print(" pressure:");
    Serial.print(pressure);
    Serial.println();

    sendData(lux, temperature, abs_pressure, pressure);
  } else {
    Serial.printf("BH1750 or BMP280 measurement not ready\n");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi disconnected");
    connectWifi();
  }

  delay(29500);
  light.configure(BH1750::ONE_TIME_HIGH_RES_MODE);
  delay(500);
}
