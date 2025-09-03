#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

typedef enum {OFF = 0, ON = 1} systemState; // state of 
typedef enum {laserDiode = 25, pushButton = 33, photoResistor = 36, LED = 17, buzzer = 2} ioPins;

// put function declarations here:
void toggleState(); // Turns on trip wire security 
void runState(); //Run the functions of said state
void calibrateLDR(); // Calibrate photoresistor to detect when the laser diode's beam is broken
bool checkBroken(); //check if photoresistor's readings are below level
void activateAlarm(); //turn on alarm signals when triggered
void deactivateAlarm(); //turn off alarm signals
void setupBLEServer(); //for setting up esp32 as a bluetooth server to receive inputs


systemState sysState;
unsigned int maxLight;
unsigned int minLight;


class MyCallbacks: public BLECharacteristicCallbacks {
void onWrite(BLECharacteristic *pCharacteristic) {
  std::string value = pCharacteristic->getValue();

  if (value == "arm") sysState = ON; // Arm
  else if (value == "disarm") sysState = OFF; // Turn Off

  if (value.length() > 0) {
    Serial.println("*********");
    Serial.print("New value: ");
    for (int i = 0; i < value.length(); i++)
    Serial.print(value[i]);
    Serial.println();
    Serial.println(sysState); //for testing purposes
    Serial.println("*********");
  }
}
void onRead(BLECharacteristic *pCharacteristic) {
  //leave blank for now; does not have usage yet but can be used for transmitting data about break-ins
}
};



void setup() {
  // set pin modes
  Serial.begin(9600);
  Serial.println("Laser Tripwire Security System Begin");
  Serial.println("Setting pins as I/O");
  pinMode(laserDiode, OUTPUT); // GPIO output for laser diode
  pinMode(pushButton, INPUT); // Push button input to activate alarm
  pinMode(photoResistor, INPUT), // GPIO input for photoresistor
  pinMode(LED, OUTPUT); // GPIO output for LED

  setupBLEServer();


  //set initial state
  systemState sysState = OFF;
  digitalWrite(laserDiode, LOW);
}

void loop() { //change to using bluetooth to turn on/off
  runState(); //looks to be okay with only reading when I input something; now I just need to allow for interrupts
  delay(250);
}



// put function definitions here:
void toggleState() {
  switch(sysState) {
    case(OFF):
      sysState = ON;
      break;
    case(ON):
      sysState = OFF;
      break;
  }
}


void runState() { //issues here with bluetooth changes
  switch(sysState) {
    case(OFF):
      digitalWrite(laserDiode, LOW);
      break;
    case(ON): //allow for disarming when pchar turns to DISARM
      digitalWrite(laserDiode, HIGH);
      calibrateLDR();
      delay(100);
      Serial.println("Activated");
      while(1) {
        if (sysState == OFF) break; //check if we disarmed the device
        if (checkBroken()) break; //check if the laser diode beam is broken
      }
      break;
  }
}


void activateAlarm() { //turn on alarm signals
  unsigned long toggleTime = millis();
  bool buzzerOn = false;
  while (!digitalRead(33) == HIGH) { //change this to only when pchar == DISARM
    unsigned long currentTime = millis();
    if (currentTime >= toggleTime) {
      toggleTime = currentTime + 500;
      digitalWrite(LED, !digitalRead(LED));
      if (buzzerOn == true) {
        buzzerOn = false;
        noTone(buzzer);
      } else {
        buzzerOn = true;
        tone(buzzer, 440);
      }
    }
  }
  deactivateAlarm();
  sysState = OFF;
}


void deactivateAlarm() { //turn off alarm signals; doing something weird with an ledc error
  digitalWrite(laserDiode, LOW);
  noTone(buzzer);
  digitalWrite(LED, LOW);
}


void calibrateLDR() { //calibrate photoresistor; set the value read with the laser diode shined on as max light
  unsigned int count = 0;
  unsigned long threeSecondTimer = millis() + 3000;

  maxLight = 0;
  minLight = INT_MAX;

  delay(200);
  while (millis() < threeSecondTimer) {
    unsigned int readVal = analogRead(photoResistor);
    Serial.println(readVal); //for testing
    if (readVal > maxLight) maxLight = readVal;
    else if (readVal < minLight) minLight = readVal;
  }
}

bool checkBroken() { //check if path between laser diode and photoresistor is broken
  unsigned int readVal = analogRead(photoResistor);
      Serial.println(readVal); //for testing
      if (readVal < maxLight - 500) {
        Serial.println(readVal); //for testing
        Serial.println("tripwire triggered");
        activateAlarm();
        return true;
      }
      return false;
}


void setupBLEServer() {
  Serial.println("Starting BLE Server");
  BLEDevice::init("LaserSecuritySystem");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("BLE Server On");
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising -> setScanResponse(true);
  pAdvertising -> setMinPreferred(0x0);
  pAdvertising -> setMinPreferred(0x02);
  BLEDevice::startAdvertising();
  Serial.println("BLE Server on Read & Write");
}
