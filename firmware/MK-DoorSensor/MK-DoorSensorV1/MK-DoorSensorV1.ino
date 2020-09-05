/*
 * Device Title: MK-DoorSensor
 * Device Description: MQTT Door Sensor
 * Device Explanation: When the magnetic switch sensor is broken then 
 *                     the device sends an mqtt message to the defined server.
 * Device information: https://www.MK-SmartHouse.com/door-sensor
 *                     
 * Author: Matt Kaczynski
 * Website: http://www.MK-SmartHouse.com
 * 
 * Code may only be distrbuted through http://www.MK-SmartHouse.com any other methods
 * of obtaining or distributing are prohibited
 * Copyright (c) 2016-2017 
 * 
 * Note: After flashing the code once you can remotely access your device by going to http://HOSTNAMEOFDEVICE.local/firmware 
 * obviously replace HOSTNAMEOFDEVICE with whatever you defined below. The user name and password are also defined below.
 */

#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* ---------- DO NOT EDIT ANYTHING ABOVE THIS LINE ---------- */

//Only edit the settings in this section

/* ==== WIFI Settings ==== */
// Name of wifi network
const char* ssid = "wifissid";

// Password to wifi network
const char* password = "wifipassword"; 


/* ==== Web Updater Settings ==== */
// Host Name of Device
const char* host = "MK-DoorSensor1";

// Path to access firmware update page (Not Neccessary to change)
const char* update_path = "/firmware";

// Username to access the web update page
const char* update_username = "admin";

// Password to access the web update page
const char* update_password = "Admin";


/* ==== MQTT Settings ==== */
// Topic which listens for commands
char* outTopic = "MK-SmartHouse/security/MK-DoorSensor1"; 

//MQTT Server IP Address
const char* server = "192.168.0.4";

//Unique device ID 
const char* mqttDeviceID = "MK-SmartHouseDevice1"; 


/* ---------- DO NOT EDIT ANYTHING BELOW THIS LINE ---------- */

//the time when the sensor outputs a low impulse
long unsigned int lowIn;         

//the amount of milliseconds the sensor has to be low 
//before we assume all detection has stopped
long unsigned int pause = 100;  

//sensor variables
boolean lockLow = true;
boolean takeLowTime;  

//the digital pin connected to the door sensor's output
int sensorPin = 13;  

//webserver 
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

//MQTT 
WiFiClient net;
MQTTClient client;

//Time Variable
unsigned long lastMillis = 0;

//Connect to WiFI and MQTT
void connect();

//Setup pins, wifi, webserver and MQTT
void setup() 
{
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, LOW);
  
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(ssid, password);
  client.begin(server, net);

  connect();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

//Connect to wifi and MQTT
void connect() 
{
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
  }

  while (!client.connect(mqttDeviceID)) 
  {
    delay(1000);
  }
}

void loop() 
{
  // MQTT Loop
  client.loop();
  delay(10);

  // Make sure device is connected
  if(!client.connected()) 
  {
    connect();
  }

  httpServer.handleClient();

  //Sensor Detection
  
  if(digitalRead(sensorPin) == HIGH)
  {
    if(lockLow)
    {  
      //makes sure we wait for a transition to LOW before any further output is made:
      lockLow = false;            
      client.publish(outTopic, "OPEN");  
      delay(50);
    }         
    takeLowTime = true;
  }

  if(digitalRead(sensorPin) == LOW)
  {       
    if(takeLowTime)
    {
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }
    //if the sensor is low for more than the given pause, 
    //we assume that no more detection is going to happen
    if(!lockLow && millis() - lowIn > pause)
    {  
      //makes sure this block of code is only executed again after 
      //a new detection sequence has been detected
      lockLow = true;                        
      client.publish(outTopic, "CLOSED");
      delay(50);
    }
  }

}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) 
{
  //This sensor does not recieve anything from MQTT Server so this is blank
}