/*
  Stepper Motor Test
  stepper-test01.ino
  Uses MA860H or similar Stepper Driver Unit
  Has speed control & reverse switch
  
  DroneBot Workshop 2019
  https://dronebotworkshop.com
  https://dronebotworkshop.com/big-stepper-motors/
*/
 
//// Defin pins (arduino)
//int reverseSwitch = 2;  // Push button for reverse
//int driverPUL = 7;    // PUL- pin
//int driverDIR = 6;    // DIR- pin
//int spd = A0;     // Potentiometer

// Defin pins (esp8266 D1 Mini)
int reverseSwitch = D7;  // Push button for reverse
int driverPUL = D6;    // PUL- pin
int driverDIR = D5;    // DIR- pin
int spd = A0;     // Potentiometer
 
// Variables
 
int pd = 500;       // Pulse Delay period
boolean setdir = LOW; // Set Direction
 
// Interrupt Handler
 
void revmotor (){
 
  setdir = !setdir;
  
}
 
 
void setup() {
 
  pinMode (driverPUL, OUTPUT);
  pinMode (driverDIR, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(reverseSwitch), revmotor, FALLING);
  
}
 
void loop() {
  
    pd = map((analogRead(spd)),0,1023,2000,50);
    digitalWrite(driverDIR,setdir);
    digitalWrite(driverPUL,HIGH);
    delayMicroseconds(pd);
    digitalWrite(driverPUL,LOW);
    delayMicroseconds(pd);
 
}
