#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "3fff1c98-3055-45b2-838e-b384b6fc1125"
#define CHARACTERISTIC_UUID "a36e7c55-6bd4-4c3c-a003-8fe819de260a"

#define bleServerName "ESP32-TEST-BLE"

bool deviceConnected = false;

unsigned long lastTime = 0;

int counter = 0;

BLECharacteristic characteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor descriptor(BLEUUID((uint16_t)0x2902));

// Setup callbacks onConnect and onDisconnect
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE");

  BLEDevice::init(bleServerName);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pService->addCharacteristic(&characteristic);
  descriptor.setValue("Counter");
  characteristic.addDescriptor(&descriptor);
  // Start the service
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  if (deviceConnected)
  {
    if ((millis() - lastTime) > 5000)
    {
      char data[4];
      dtostrf(counter, 4, 2, data);
      characteristic.setValue(data);
      characteristic.notify();

      lastTime = millis();
      counter += 1;
    }
  }
}
