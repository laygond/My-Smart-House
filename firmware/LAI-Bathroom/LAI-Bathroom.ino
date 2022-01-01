/*
 * < LAYGOND-AI >
 * Device Title: LAI-Bathroom (Version 1 - Cheapest)
 * Device Description: - Play Deadining White Noise Sound while bathroom is in use preventing
 *                       indoor sounds from being identified.
 *                     - Side-Feature: Activate bathroom lights on motion. 
 * 
 * Device Explanation: This version uses: PIR, door contact sensor, relay, and esp8266 wemos D1 mini. 
 *                     Lights are activated by motion. However, if lights ON and door closed then 
 *                     lights are locked as ON (ignore motion) and play white noise until door 
 *                     opens. This setup is therefore limited by the choice of sensors and cannot 
 *                     determine if the door is closed from the inside or outside. To deactivate system,
 *                     leave door open once using the bathroom.
 *                     
 * Device Improvement: To determine if door is being closed from the inside: add an ultrasonic range sensor  
 *                     or TimeOfFlight Laser Range sensor to detect people inside. Or replace PIR with
 *                     two range sensor (before and after door) to know if bathroom is being occupied or not
 *                     
 *                     
 * Author: Bryan Laygond
 * Github: @laygond
 * 
 * Inspired by: 
 * Bill https://dronebotworkshop.com/
 * 
 * Code may only be distributed through https://github.com/laygond/Elevator-AI any 
 * other methods of obtaining or distributing are prohibited.
 * < LAYGOND-AI > Copyright (c) 2020
 * 
 */
// NOTE: PIR has been set to L mode with a high state time delay of 30~40 seconds after that
//       it needs 6 seconds to recheck its state.

MOTION_PIN = ;
LIGHTS_PIN= ;
DOOR_PIN = ;
SOUND_PIN= LED_BUILTIN; // (represented by LED FOR NOW)
lightsON = false; // assume lights are OFF
currentDoorState = false;  // assume door OPEN 

void setup()
{
  pinMode(MOTION_PIN, INPUT);
  pinMode(LIGHTS_PIN, OUTPUT);
  pinMode(DOOR_PIN, INPUT);
  pinMode(SOUND_PIN, OUTPUT);
  delay(60000)  // Wait one minute for pir motion sensor to adapt
}

void loop() 
{
  //======= MOTION -> LIGHTS ON =======
  checkMotion = true;    //whether motion sensor should be checked in the future true:YES false:NO
  start6SecWait = false; //whether 6 second timer has been initilized true:YES false:NO
  motionStateON = measure(MOTION_PIN);
  if (motionStateON == true){ 
      digitalWrite(LIGHTS_PIN, HIGH); //Turn ON Lights
      lightsON == true;}
  
  //=======  DOOR -> SOUND ON/OFF =======
  while (lightsON== true){
      //Measure door event
      lastDoorState = currentDoorState;       // save the last state
      currentDoorState  = measure(DOOR_PIN);  // read new state
      if (lastDoorState == false && currentDoorState == true) { // state change: OPEN -> CLOSED
          digitalWrite(SOUND_PIN, HIGH);//Sound signal ON 
          checkMotion = false;}         //Keep Bathroom state on hold until door opens again
      if (lastDoorState == true && currentDoorState == false) { // state change: CLOSED -> OPEN
          delay (4000); //milisec
          digitalWrite(SOUND_PIN, LOW);//Sound signal OFF 
          checkMotion = true;
          start6SecWait = false;}
      
      //===== NO MOTION -> NO LIGHTS ====     
      //While door is open check motion
      if (checkMotion == true){
          motionStateON = measure(MOTION_PIN);
          if (motionStateON == false){ 
              //Re-check in 6 seconds to confirm since PIR needs time between transitions
              if (start6SecWait == false){// Has timer been initialized ?
                  timer = millis();
                  start6SecWait = true;}
              if (start6SecWait == true && milis()- timer >6){// Has 6 sec passed and still no motion?
                  digitalWrite(LIGHTS_PIN, LOW); //Turn OFF Lights
                  lightsON == false;}}           //Exit while loop and await next visitor
          else{// There is still motion
              start6SecWait = false;}}} //reset, motion is back: it was just the transition      
}
