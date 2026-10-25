#include "BLEDevice.h"

#define bleServerName "ESP32-TEST-BLE"

static BLEUUID serviceUUID("3fff1c98-3055-45b2-838e-b384b6fc1125");
static BLEUUID characteristicUUID("a36e7c55-6bd4-4c3c-a003-8fe819de260a");

static boolean doConnect = false;
static boolean connected = false;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic *characteristic;

const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

char *counter;

boolean newCounter = false;

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress)
{
  BLEClient *pClient = BLEDevice::createClient();

  pClient->connect(pAddress);
  Serial.println("Connected to server");

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return (false);
  }

  characteristic = pRemoteService->getCharacteristic(characteristicUUID);

  if (characteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");

  // Assign callback functions for the Characteristics
  characteristic->registerForNotify(counterNotifyCallback);
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.getName() == bleServerName)
    {
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      Serial.println("Device found. Connecting!");
    }
  }
};

static void counterNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                  uint8_t *pData, size_t length, bool isNotify)
{
  counter = (char *)pData;
  newCounter = true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting ble client");

  BLEDevice::init("");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop()
{
  if (doConnect == true)
  {
    if (connectToServer(*pServerAddress))
    {
      Serial.println("Connected to server");
      // Activate the Notify property of each Characteristic
      characteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)notificationOn, 2, true);
      connected = true;
    }
    else
    {
      Serial.println("Failed to connect to server");
    }
    doConnect = false;
  }
  if (newCounter)
  {
    newCounter = false;
    Serial.println(counter);
  }
  delay(1000);
}
