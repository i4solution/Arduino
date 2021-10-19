#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristicW = NULL;
BLECharacteristic* pCCCD = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

//Clone HM10

#define SERVICE_UUID        "0000FFE0-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_R_UUID "0000FFE4-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_W_UUID "0000FFE9-0000-1000-8000-00805F9B34FB"
#define BLUETOOTH_LE_CCCD "00002902-0000-1000-8000-00805f9b34fb"
//
const int ledPin =  GPIO_NUM_2;
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete
bool ledOn = false;  // whether the string is complete


class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connected Devcie : 1");
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("Disconnected Devcie : ");
      deviceConnected = false;
    }
};

class WriteCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
        {
         // Serial.print(rxValue[i]);
          char inChar = (char)rxValue[i];
          // add it to the inputString:
          inputString += inChar;
          // if the incoming character is a newline, set a flag so the main loop can
          // do something about it:
          if (inChar == '\n') {
            stringComplete = true;
          }
        }
        Serial.println();
        Serial.println("*********");
      }
    }
};

void blePrint(std::string value) {
  pCharacteristicW->setValue(value);
  pCharacteristicW->notify();
  delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32BLE");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_R_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

   pCharacteristicW = pService->createCharacteristic(
                      CHARACTERISTIC_W_UUID,
                      BLECharacteristic::PROPERTY_WRITE   |                      
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

//          pCCCD = pService->createCharacteristic(
//                      BLUETOOTH_LE_CCCD,
//                      BLECharacteristic::PROPERTY_READ   |
//                      BLECharacteristic::PROPERTY_WRITE   |                      
//                      BLECharacteristic::PROPERTY_NOTIFY
//                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  //pCharacteristic->addDescriptor(new BLE2902());
  //pCharacteristic->setCallbacks(new WriteCallbacks());

  pCharacteristic->addDescriptor(new BLE2902());
  //pCharacteristic->setCallbacks(new WriteCallbacks());

  pCharacteristicW->setValue("Hello World BLE");
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (stringComplete) {
    inputString.trim();
    Serial.println(inputString);
    if (inputString == "ON") {
      digitalWrite(ledPin, HIGH);
      blePrint("ON\r\n");
      ledOn = true;
    }
    else if (inputString == "OFF") {
      digitalWrite(ledPin, LOW);
      blePrint("OFF\r\n");
      ledOn = false;
    }
    else if (inputString == "Sync") {
      if (ledOn)
        blePrint("ON\r\n");
      else
        blePrint("OFF\r\n");
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
    delay(20);//Time for Bluetooth stack handle
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
