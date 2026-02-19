#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "HX711.h"

#define LOADCELL_DOUT_PIN 23
#define LOADCELL_SCK_PIN  22

#define SERVICE_UUID               "d4d61900-3739-4da1-8f5c-d973f6805c36"
#define SENSOR_CHAR_UUID           "d4d61901-3739-4da1-8f5c-d973f6805c36"
#define TARE_CHAR_UUID             "d4d61902-3739-4da1-8f5c-d973f6805c36"
#define CAL_CHAR_UUID              "d4d61903-3739-4da1-8f5c-d973f6805c36"

HX711 scale;

float calibration_factor = -10160.0;

BLEServer* server;
BLECharacteristic* sensorChar;
BLECharacteristic* tareChar;
BLECharacteristic* calChar;

bool deviceConnected = false;
bool oldDeviceConnected = false;

volatile bool requestTare = false;
volatile bool requestCalUpdate = false;
float newCalibrationFactor = 0.0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* s) {
    deviceConnected = false;
  }
};

class TareCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) {
    uint8_t* data = c->getData();
    if (data && data[0] == 1) {
      requestTare = true;
    }
  }
};

class CalibrationCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) {
    int len = c->getLength();
    if (len > 0 && len < 10) {
      char buf[10];
      memcpy(buf, c->getData(), len);
      buf[len] = '\0';
      newCalibrationFactor = atof(buf);
      requestCalUpdate = true;
    }
  }
};

void setup() {
  Serial.begin(115200);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  BLEDevice::init("ReGrip");

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(SERVICE_UUID);

  sensorChar = service->createCharacteristic(
    SENSOR_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  sensorChar->addDescriptor(new BLE2902());

  tareChar = service->createCharacteristic(
    TARE_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  tareChar->setCallbacks(new TareCallbacks());

  calChar = service->createCharacteristic(
    CAL_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  calChar->setCallbacks(new CalibrationCallbacks());

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(false);
  adv->setMinPreferred(0x00);
  BLEDevice::startAdvertising();
}

void loop() {

  if (deviceConnected) {

    if (requestCalUpdate) {
      calibration_factor = newCalibrationFactor;

      Serial.println(calibration_factor);
      scale.set_scale(calibration_factor);
      requestCalUpdate = false;
    }

    if (requestTare) {
      scale.tare();
      requestTare = false;
    }

    float weight = scale.get_units(5);
    sensorChar->setValue(String(weight).c_str());
    sensorChar->notify();

    delay(100);
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    server->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
