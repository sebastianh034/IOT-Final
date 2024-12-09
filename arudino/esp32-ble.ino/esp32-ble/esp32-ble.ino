#include <BLEDevice.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <Adafruit_MLX90614.h>

#define DEVICE_NAME         "Sebastian BLE"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define MLX_SDA 14 // This is your data pin
#define MLX_SCL 13 // This is your clock pin

#define BUTTON 3 // button pin to activate relay
#define LIGHT 21 // GPIO pin for the light
#define RELAY 23 //relay 

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // Address, frequency, I2C group, resolution, reset
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); 

BLECharacteristic *pCharacteristic;
String message = "";
float savedTemperature = 0.0;

bool relayActive = false; 
unsigned long relayStartTime = 0;

void printToScreen(String s) {
  display.clear();
  display.drawString(0, 0, s);
  display.display();
}

// Function to read temperature from the MLX90614 sensor
float readTemperature() {
  return mlx.readObjectTempF();
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    printToScreen("BLE client connected.");
  }

  void onDisconnect(BLEServer* pServer) {
    printToScreen("BLE client disconnected.");
    BLEDevice::startAdvertising();
  }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) {
    message = String(characteristic->getValue().c_str());
    printToScreen("Received:\n" + message);
    //if it recived the message "spray" from the app it will turn on the light and the relay
    if (message == "spray") {
      digitalWrite(RELAY, LOW);
      digitalWrite(LIGHT, HIGH);  // turn on the light 
      printToScreen("Spraying for 5 seconds");

      delay(5000); // Spray for 5 seconds

      digitalWrite(RELAY, HIGH);
      digitalWrite(LIGHT, LOW);  // turn off the LIGHT 
      printToScreen("Spraying complete");
    }
  }
};

void setup() {
  pinMode(0, INPUT_PULLUP);
  Serial.begin(9600);
  
  bool wireStatus = Wire1.begin(MLX_SDA, MLX_SCL);
  bool mlxStatus = mlx.begin(MLX90614_I2CADDR, &Wire1);
  
  if (!mlxStatus) {
    Serial.println("Failed to find MLX90614 sensor");
    while (1);
  }

  display.init();
  
  pinMode(RELAY, OUTPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  // Turn off all lights initially
  digitalWrite(LIGHT, LOW); 
  digitalWrite(RELAY, HIGH); //set as high == low

  printToScreen("Starting BLE!");

  BLEDevice::init(DEVICE_NAME);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->setValue("Init");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void loop() {
  // Read temperature
  float objectTemp = readTemperature();
  Serial.println("Temperature: " + String(objectTemp) + " °F");
  printToScreen("Temp:\n" + String(objectTemp) + " F");
  pCharacteristic->setValue(String(objectTemp).c_str());
  pCharacteristic->notify();

  // If temperature is above 140F, turn on the relay only if the relay is not active
  if (objectTemp > 140.0 && !relayActive && digitalRead(BUTTON) == HIGH) {
    digitalWrite(RELAY, LOW); //low == on
    digitalWrite(LIGHT, HIGH);
    printToScreen("Temperature above 140°F. Spraying...");
    delay(5000);
    relayStartTime = millis(); 
    relayActive = true;        
  }

  // Manage the relay for 5 seconds
  if (relayActive && (millis() - relayStartTime >= 5000)) {
    digitalWrite(LIGHT, LOW);  // Turn off the light
    digitalWrite(RELAY, HIGH); // relay off
    printToScreen("Spraying complete.");
    relayActive = false;       // Reset the relay active flag
  }

  if (savedTemperature != 0.0) {
    pCharacteristic->setValue(String(savedTemperature).c_str());
    pCharacteristic->notify();
  }

  // Check if the button is pressed
  if (digitalRead(BUTTON) == LOW) {
    digitalWrite(LIGHT, HIGH);  // turn on the light
    digitalWrite(RELAY, LOW); //turn on relay
    printToScreen("Spraying...");

    // Wait until the button is released
    while (digitalRead(BUTTON) == LOW) {
      delay(10);
    }
    digitalWrite(RELAY, HIGH);
    digitalWrite(LIGHT, LOW);  // turn off the light

    printToScreen("Spraying stopped.");
  }

  else {
    digitalWrite(LIGHT, LOW); // check the light is off if the button is not pressed
    digitalWrite(RELAY, HIGH);
  }

  delay(10);
}
