#include <Arduino.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

/* Time Constants */
#define BUZZ_TIME 15000
#define SLEEP 20

/* Service UUIDs */
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

/* PIR Sensor Variables*/
bool isActive = false;
bool pirEnabled = true;
unsigned long activeStartTime;
int value = 0;
int lastPirValue = HIGH;

/* Pin Definitions */
#define RED_LED 17
#define BUZZER 27
#define PIR 26

/* BLE Definitions */
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* p) override {
    Serial.println(">>> BLE client connected");
  }
  void onDisconnect(BLEServer* p) override {
    Serial.println(">>> BLE client disconnected");
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(PIR, INPUT);
  activeStartTime = millis();

  Serial.println("Waiting for PIR to settle…");
  delay(30000);                 
  while (digitalRead(PIR) == HIGH) {
    delay(50);                   
  }
  Serial.println("PIR ready!");

  digitalWrite(RED_LED, LOW);

  BLEDevice::init("SDSU_FINAL_RING_LITE");
  Serial.print("BLE MAC: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
    );
  pCharacteristic->setValue("Server Example -- SDSU IOT");
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); 
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0); 
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  pCharacteristic->notify();
  Serial.println("Starting BLE work!");
}

void loop() {
  delay(300);
  value = digitalRead(PIR);
  delay(10);
  Serial.print(value);

  // >>> Rising‑edge test (LOW → HIGH)
  bool risingEdge = (lastPirValue == LOW && value == HIGH);
  lastPirValue = value;      // remember current value for next pass
  // <<<

  if (((millis() - activeStartTime) > BUZZ_TIME) && isActive) {
    isActive = false;
    Serial.println("Buzzer turned off...");
    analogWrite(BUZZER, 0);
    sleep(SLEEP);
    // BLEDevice::stopAdvertising();
  }

  // >>> use risingEdge instead of raw value
  if (risingEdge && !isActive) {
  // <<<
    isActive = true;
    activeStartTime = millis();
    // BLEDevice::startAdvertising();
    Serial.println("Buzzer turned on...");
    pCharacteristic->setValue("Motion");
    pCharacteristic->notify();
    delay(1000);
    pCharacteristic->setValue("No Motion");
    pCharacteristic->notify();
  }

  if ((millis() % 500 < 250) && isActive) {
    analogWrite(BUZZER, 10);
    digitalWrite(RED_LED, HIGH);
  } else {
    analogWrite(BUZZER, 0);
    digitalWrite(RED_LED, LOW);
  }
  delay(100);
}
