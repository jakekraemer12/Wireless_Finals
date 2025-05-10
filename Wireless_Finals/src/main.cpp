/* Libraries */
#include <Arduino.h>
#include <BLEDevice.h>      // Core Ardunio API
#include <BLEUtils.h>       // BLE Services
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

/* Callback class used as Serial debug for BLE connection to raspberry pi */
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

  // Initialize I/O Pins
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(PIR, INPUT);

  // Begin timer for active start
  activeStartTime = millis();

  // Allow PIR to settle: used to avoid the initial read being motion
  Serial.println("Waiting for PIR to settle…");
  delay(30000);                 
  while (digitalRead(PIR) == HIGH) {
    delay(50);                   
  }
  Serial.println("PIR ready!");

  // Turn BUZZER and LED off
  digitalWrite(RED_LED, LOW);
  analogWrite(BUZZER, 0);

  // Create BLE subsystem
  BLEDevice::init("SDSU_FINAL_WIRELESS");

  // Debug to grab BLE MAC for ESP32 to use for bash script on raspberrypi
  Serial.print("BLE MAC: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());

  // Create server, services, and characteristics
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
    );

  // Initialize value for Characteristic
  pCharacteristic->setValue("Wireless -- Motion Detection Camera");
  pService->start();

  // Begin advertising the service UUID
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
  // Read PIR Sensor
  value = digitalRead(PIR);
  delay(10);

  // Stream raw PIR sensor data to serial for debug
  Serial.print(value);

  // Determine if motion is rising edge
  bool risingEdge = (lastPirValue == LOW && value == HIGH);

  // Remember value for next pass
  lastPirValue = value;

  // Handle end of motion
  if (((millis() - activeStartTime) > BUZZ_TIME) && isActive) {
    // Reset state
    isActive = false;
    Serial.println("Buzzer turned off...");

    // Turn BUZZER and LED off
    analogWrite(BUZZER, 0);
    sleep(SLEEP);
  }

  // Handle start of motion at rising edge
  if (risingEdge && !isActive) {
    // Set state to active grab cuurent time for BUZZER and LED control
    isActive = true;
    activeStartTime = millis();
    
    // Update characteristic to display "Motion"
    Serial.println("Buzzer turned on...");
    pCharacteristic->setValue("Motion");
    pCharacteristic->notify();
    delay(1000);

    // Reset characteristic
    pCharacteristic->setValue("No Motion");
    pCharacteristic->notify();
  }

  // Turn on LED and Buzzer for alloated time while motion state is active
  if ((millis() % 500 < 250) && isActive) {
    analogWrite(BUZZER, 10);
    digitalWrite(RED_LED, HIGH);
  // Keep silent when in iactive state
  } else {
    analogWrite(BUZZER, 0);
    digitalWrite(RED_LED, LOW);
  }
  delay(100);
}

