#include <Arduino.h>

typedef enum {OFF = 0, ON = 1} systemState; // state of 
typedef enum {laserDiode = 25, pushButton = 33, photoResistor = 36, LED = 17, buzzer = 2} ioPins;

// put function declarations here:
void toggleState(); // Turns on trip wire security 
void runState(); //Run the functions of said state
void armSystem();
void calibrateLDR(); // Calibrate photoresistor to detect when the laser diode's beam is broken
void activateAlarm();
void deactivateAlarm();


systemState sysState;
unsigned int maxLight;
unsigned int minLight;

//NOTE: everything works; however it takes 3 button presses to get out of activateAlarm state
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
      deactivateAlarm();
      break;
    case(ON):
      digitalWrite(laserDiode, HIGH);
      calibrateLDR();
      delay(100);
      Serial.println("Activated");
      while(1) {
        unsigned int readVal = analogRead(photoResistor);
        Serial.println(readVal); //for testing
        if (readVal < maxLight - 500) { //could use some major work
          Serial.println(readVal); //for testing
          Serial.println("tripwire triggered");
          activateAlarm();
          break;
        }
      }
      break;
      //check for a trigger
      //run actions for a trigger
      //exit

  }
}


void activateAlarm() {
  unsigned long toggleTime = millis();
  bool buzzerOn = false;
  while (!digitalRead(33) == HIGH) {
    unsigned long currentTime = millis();
    if (currentTime >= toggleTime) {
      toggleTime = currentTime + 500;
      digitalWrite(LED, !digitalRead(LED));
      if (buzzerOn == true) { //can change to make it play two different tones
        buzzerOn = false;
        noTone(buzzer);
      } else {
        buzzerOn = true;
        tone(buzzer, 440);
      }
    }
  }
  deactivateAlarm();
}


void deactivateAlarm() {
  digitalWrite(laserDiode, LOW);
  noTone(buzzer);
  digitalWrite(LED, LOW);
}


void calibrateLDR() {
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

