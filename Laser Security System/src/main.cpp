#include <Arduino.h>

typedef enum {OFF = 0, ON = 1} systemState; // state of 
typedef enum {laserDiode = 25, pushButton = 33, photoResistor = 21, LED = 36, buzzer = 37} ioPins;

// put function declarations here:
void toggleSystem(); // Turns on trip wire security system
void calibrateLDR(); // Calibrate photoresistor to detect when the laser diode's beam is broken


systemState sysState = OFF;

void setup() {
  // set pin modes
  pinMode(laserDiode, OUTPUT); // GPIO output for laser diode
  pinMode(pushButton, INPUT); // Push button input to activate alarm
  pinMode(photoResistor, INPUT), // Photoresistor

  pinMode(LED, OUTPUT); // Pin for test LED

  //set initial state
  digitalWrite(laserDiode, LOW);
}

void loop() {
  if (digitalRead(33) == HIGH) {
    toggleSystem();
    delay(250);
  }
}

// put function definitions here:
void toggleSystem() {
  switch(sysState) {
    case(OFF):
      sysState = ON;
    case(ON):
      sysState = OFF;
  }
  digitalWrite(laserDiode, !digitalRead(laserDiode));
}