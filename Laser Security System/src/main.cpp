#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

//for creating an HTTPS application 
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string>

// for connecting to WiFi and IoT Hub (NOTE: REMOVE BEFORE PUSHING)
#define WIFI_SSID "" // NOTE: Please delete this value before submitting assignment
#define WIFI_PASSWORD "" // NOTE: Please delete this value before submitting assignment

// Azure IoT Hub configuration
#define SAS_TOKEN ""
// Root CA certificate for Azure IoT Hub
const char* root_ca = "";


String iothubName = "LaserSecuritySystem"; //Your hub name (replace if needed)
String deviceName = "LaserSecuritySystem"; //Your device name (replace if needed)
String url = "https://" + iothubName + ".azure-devices.net/devices/" + 
deviceName + "/messages/events?api-version=2021-04-12";


// states and pins for the laser security system
typedef enum {OFF = 0, ON = 1} systemState; // state of 
typedef enum {laserDiode = 25, pushButton = 33, photoResistor = 36, LED = 17, buzzer = 2} ioPins;

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// function declarations
void toggleState(); // Turns on trip wire security 
void runState(); //Run the functions of said state
void calibrateLDR(); // Calibrate photoresistor to detect when the laser diode's beam is broken
bool checkBroken(); //check if photoresistor's readings are below level
void activateAlarm(); //turn on alarm signals when triggered
void deactivateAlarm(); //turn off alarm signals
void connectToWiFi();
void configureDS1307();
bool getTime(const char *str);
bool getDate(const char *str);
void printTime();
void print2digits(int number);

//global variables for laser security system
systemState sysState;
unsigned int maxLight;
unsigned int minLight;
tmElements_t tm;



void setup() {
  // set pin modes
  Serial.begin(9600);
  Serial.println("Laser Tripwire Security System Begin");
  Serial.println("Setting pins as I/O");
  pinMode(laserDiode, OUTPUT); // GPIO output for laser diode
  pinMode(pushButton, INPUT); // Push button input to activate alarm
  pinMode(photoResistor, INPUT), // GPIO input for photoresistor
  pinMode(LED, OUTPUT); // GPIO output for LED

  // connect to WiFi
  //connectToWiFi();

  //configure DS1307 RTC
  configureDS1307();

  //set initial state
  systemState sysState = OFF;
  digitalWrite(laserDiode, LOW);
}

void loop() {
  if (digitalRead(33) == HIGH) {
    toggleState();
    runState();
    delay(250); //delay to not overrun this; can remove
  }
}



// function definitions
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


void runState() { // main function; runs actions of each state
  switch(sysState) {
    case(OFF):
      digitalWrite(laserDiode, LOW); // turn off laser diode
      break;
    case(ON):
      digitalWrite(laserDiode, HIGH);
      calibrateLDR(); // when turning to state ON, must calibrate LDR to be able to detect laser diode beam breaks
      delay(100);
      Serial.println("Activated"); 
      while(1) {
        //if (sysState == OFF) break; // check if we disarmed the device
        if (checkBroken()) break; // check if the laser diode beam is broken
      }
      break;
  }
}


void activateAlarm() { // turn on alarm signals
  unsigned long toggleTime = millis();
  bool buzzerOn = false;
  while (!digitalRead(33) == HIGH) { // disarm only with button press not using bluetooth connection "disarm" command
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
  deactivateAlarm(); // when the button is pressed, deactivate the alarms
  sysState = OFF;
}


void deactivateAlarm() { //turn off alarm signals; doing something weird with an ledc error
  digitalWrite(laserDiode, LOW);
  noTone(buzzer);
  digitalWrite(LED, LOW);
}


void calibrateLDR() { // calibrate photoresistor; set the value read with the laser diode shined on as max light
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


bool checkBroken() { // check if path between laser diode and photoresistor is broken
  unsigned int readVal = analogRead(photoResistor);
      Serial.println(readVal); //for testing
      if (readVal < maxLight - 500) {
        Serial.println(readVal); //for testing
        Serial.println("tripwire triggered");
        
        //print the time break in happened; this will be changed to sending to cloud later
        printTime();

        activateAlarm();
        return true;
      }
      return false;
}


void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MAC address: ");
    Serial.println(WiFi.macAddress());
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}


void configureDS1307() {
  bool parse=false;
  bool config=false;

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }
  delay(200); // confirmation for configuring RTC correctly
  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);
  } else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");
  } else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
  }
}


bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}


bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void printTime() {
  if (RTC.read(tm)) {
    Serial.print("Ok, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    delay(9000);
  }
  delay(1000);
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}