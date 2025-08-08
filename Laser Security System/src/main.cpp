#include <Arduino.h>

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  pinMode(25, OUTPUT); //GPIO output for laser diode
  pinMode(33, INPUT); //Push button input to activate alarm
  digitalWrite(25, HIGH);
}

void loop() {
  if (digitalRead(33) == HIGH) {
    digitalWrite(25, !digitalRead(25));
    delay(250);
  }
}

// put function definitions here: