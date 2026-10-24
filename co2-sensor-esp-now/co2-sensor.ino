#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <esp_now.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include "TM1637.h"

SCD30 airSensor;

RTC_DATA_ATTR int counter = 0;

const char *ssid = "";
const char *password = "";
String endpoint_88 = "";
String endpoint_e0 = "";
String endpoint_fc = "";

const int CLK = D2;
const int DIO = D3;

TM1637 tm(CLK, DIO);

struct msg_data_e0
{
  float lux;
  int moisture;
  float vBat;
  int num_tries;
};

struct msg_data_fc
{
  float pressure;
  float temperature;
  int num_tries;
};

struct msg_data_e0 data_e0 = {-5, -5, -5, 0};
struct msg_data_fc data_fc = {0.0, 0.0, 0};

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  Serial.printf("Received message from %02x\n", mac_addr[5]);
  if (mac_addr[5] == 224)
  {
    memcpy(&data_e0, incomingData, sizeof(msg_data_e0));
    Serial.printf("lux: %f, moisture: %d, vBat: %f, retries: %d\n", data_e0.lux, data_e0.moisture, data_e0.vBat, data_e0.num_tries);
  }
  else if (mac_addr[5] == 252)
  {
    memcpy(&data_fc, incomingData, sizeof(msg_data_fc));
    Serial.printf("pressure: %f, temperature: %f, retries: %d\n", data_fc.pressure, data_fc.temperature, data_fc.num_tries);
  }
}

void connectWifi()
{
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

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void connectSensor()
{
  Wire.begin();

  while (airSensor.begin() == false)
  {
    Serial.println("Sensor not detected!");
    delay(1000);
  }
  airSensor.setTemperatureOffset(1.8);

  delay(1000);
}

void setup()
{
  Serial.begin(115200);
  tm.begin();
  tm.setBrightness(1);

  connectSensor();
  connectWifi();
}

void loop()
{
  uint16_t co2 = 0;
  float temp = 0.0;
  float hum = 0.0;

  if (airSensor.dataAvailable())
  {
    co2 = airSensor.getCO2();
    temp = airSensor.getTemperature();
    hum = airSensor.getHumidity();

    int offset = 1;
    if (co2 >= 1000)
    {
      offset = 0;
    }
    tm.display(co2, true, false, offset);

    Serial.print("co2(ppm):");
    Serial.print(co2);

    Serial.print(" temp(C):");
    Serial.print(temp);

    Serial.print(" humidity(%):");
    Serial.print(hum);

    Serial.println();
  }
  else
  {
    Serial.println("Waiting for new data");
  }

  if (WiFi.status() == WL_CONNECTED && co2 != 0)
  {
    HTTPClient http;

    String co2_prom = "#TYPE home_co2 gauge\nhome_co2 " + String(co2) + "\n";
    String hum_prom = "#TYPE home_humidity gauge\nhome_humidity " + String(hum) + "\n";
    String temp_prom = "#TYPE home_temperature gauge\nhome_temperature " + String(temp) + "\n";

    String body = co2_prom + hum_prom + temp_prom;

    http.begin(endpoint_88.c_str());

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

  if (WiFi.status() == WL_CONNECTED && data_e0.lux > -4)
  {
    HTTPClient http;

    String lux_e0 = "#TYPE home_lux gauge\nhome_lux " + String(data_e0.lux) + "\n";
    String moisture_e0 = "#TYPE home_moisture gauge\nhome_moisture " + String(data_e0.moisture) + "\n";
    String vBat_e0 = "#TYPE home_e0_vBat gauge\nhome_e0_vBat " + String(data_e0.vBat) + "\n";

    data_e0 = {-5, -5, -5, 1};

    String body = lux_e0 + moisture_e0 + vBat_e0;

    http.begin(endpoint_e0.c_str());

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

  if (WiFi.status() == WL_CONNECTED && data_fc.pressure > 0)
  {
    HTTPClient http;

    String pressure_fc = "#TYPE home_outside_pressure gauge\nhome_outside_pressure " + String(data_fc.pressure) + "\n";
    String temperature_fc = "#TYPE home_outside_temperature gauge\nhome_outside_temperature " + String(data_fc.temperature) + "\n";

    data_fc = {0, 0, 0};

    String body = pressure_fc + temperature_fc;

    http.begin(endpoint_fc.c_str());

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

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi disconnected");
    connectWifi();
  }

  counter += 1;

  delay(30000);
}
