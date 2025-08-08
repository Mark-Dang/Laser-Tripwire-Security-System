#include <Arduino.h>

typedef enum {OFF = 0, ON = 1} systemState; // state of 
typedef enum {laserDiode = 25, pushButton = 33, photoResistor = 36, LED = 22, buzzer = 37} ioPins;

// put function declarations here:
void toggleState(); // Turns on trip wire security 
void runState(); //Run the functions of said state
void armSystem();
void calibrateLDR(); // Calibrate photoresistor to detect when the laser diode's beam is broken


systemState sysState;
unsigned int maxLight;
unsigned int minLight;

void setup() {
  // set pin modes
  Serial.begin(9600);
  Serial.println("Laser Tripwire Security System Begin");
  pinMode(laserDiode, OUTPUT); // GPIO output for laser diode
  pinMode(pushButton, INPUT); // Push button input to activate alarm
  pinMode(photoResistor, INPUT), // GPIO input for photoresistor
  pinMode(LED, OUTPUT); // GPIO output for LED


  //set initial state
  systemState sysState = OFF;
  digitalWrite(laserDiode, LOW);
}

void loop() {
  if (digitalRead(33) == HIGH) {
    toggleState();
    runState();
    delay(250);
  }
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


void runState() {
  switch(sysState) {
    case(OFF):
      digitalWrite(laserDiode, LOW);
      digitalWrite(LED, LOW);
      break;
    case(ON):
      digitalWrite(laserDiode, HIGH);
      calibrateLDR();
      while(1) {
        unsigned int readVal = analogRead(photoResistor);
        if (readVal < (maxLight + minLight) / 2) {
          Serial.println(readVal);
          Serial.println("tripwire triggered");
          break;
        }
        break;
      }
      //check for a trigger
      //run actions for a trigger
      //exit

  }
}


void calibrateLDR() {
  unsigned int count = 0;
  unsigned long fiveSecondTimer = millis() + 5000;

  maxLight = 0;
  minLight = INT_MAX;

  digitalWrite(LED, HIGH);
  while (millis() < fiveSecondTimer) {
    unsigned int readVal = analogRead(photoResistor);
    if (readVal > maxLight) maxLight = readVal;
    else if (readVal < minLight) minLight = readVal;
  }
  digitalWrite(LED, LOW);
  Serial.println(maxLight); //testing
  Serial.println(minLight);
}

