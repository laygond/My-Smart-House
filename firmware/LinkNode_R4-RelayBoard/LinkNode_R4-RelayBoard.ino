/**
 * MQTT Relay Board: When the device receives an mqtt message
 * it triggers ON or OFF the specified relay switch, e.g., if
 * the message is R1ON then it turns on relay #1
 * Inspired by Matt Kaczynski http://www.MK-SmartHouse.com
 *          and SteveInCo https://github.com/SteveInCO
 * Author Bryan Laygond
 * 
 * NETWORK AND SERVER SET-UP:
 * The MQTT Relay Board connects automatically to your local network and
 * MQTT server if a configuration file is provided. If file not provided,
 * it allows for a manual wireless web configuration: search the device's
 * WiFi SSID (from phone, laptop, etc) and then fill the configuration
 * panel via web browser at 192.168.4.1
 * 
 * REMOTE ACCESS:
 * Once connected to your network you can access your device by going to
 * http://HOSTNAMEOFDEVICE.local or http://YOUR_DEVICE_IP. Update of 
 * firmware can be done remotely(manually) at http://YOUR_DEVICE_IP/firmware 
 * 
 * HARDWARE:
 * ESP8266 ESP-12F connected as LinkNode R4 V1.0 
 * https://www.linksprite.com/wiki/index.php?title=LinkNode_R4:_Arduino-compatible_WiFi_relay_controller
 *    
 * PREREQUISITES:
 * In Arduino IDE Install the following:
 * Under 'Sketch/Include Library/Manage Libraries'
 * - ArduinoJson by Benoit Blanchon   Version 5.13.5 (V5 is a must)
 * - MQTT        by Joel Gaehwiler    Version 2.4.7
 * - WiFiManager by tzapu             Version 2.0.3  
 * 
 * Under 'File/Preferences/Additional Board Manage URL' include
 * - http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * 
 * Under 'Tools/Boards/Board Manager'
 * - esp8266     by ESP8266 Community Version 2.5.0
 * 
 * UPLOAD SET-UP:
 * In the physical hardware set the jumper pins to 'PGM via UART'
 * once upload is completed set jumper pins back to 'boot from flash'
 * In Arduino IDE set: 
 *    Board: "Generic ESP8266 Module"
 *    Upload Speed: "115200" (or lower)
 *    Flash Size: "512K (64K SPIFFS)" 
 *    Flash Mode "QIO" (since ESP-12F)
 *    Reset Method: "ck" (since ESP-12F)
 * 
 **/

#include <ESP8266WiFi.h>
#include <MQTTClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* ---------- DO NOT EDIT ANYTHING ABOVE THIS LINE ---------- */

//Only edit the settings in this section

/* WIFI Settings */
// Name of wifi network
const char* ssid = "WiFi";

// Password to wifi network
const char* password = "password"; 

/* Web Updater Settings */
// Host Name of Device
const char* host = "MK-SonoffPowerStrip1";

// Path to access firmware update page (Not Neccessary to change)
const char* update_path = "/firmware";

// Username to access the web update page
const char* update_username = "admin";

// Password to access the web update page
const char* update_password = "Admin";

/* MQTT Settings */
// Topic which listens for commands
char* subscribeTopic = "MK-SmartHouse/utilities/MK-SonoffPowerStrip1"; 

//MQTT Server IP Address
const char* server = "192.168.0.4";

//Unique device ID 
const char* mqttDeviceID = "MK-SmartHouseDevice1"; 


/* ---------- DO NOT EDIT ANYTHING BELOW THIS LINE ---------- */

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

int relays[] = { 14, 12, 13, 16 };  // GPIO pins for each relay
boolean relayStates[] = { false, false, false, false };  // Initial state of each relay


//bool channel1 = false;
//bool channel2 = false;

void connect();

void setup() 
{
  Serial.begin(19200);

  for (size_t i = 0; i < sizeof(relays) / sizeof(int); i++)
  {  
    pinMode(relays[i], OUTPUT);
    digitalWrite(relays[i], deviceStates[i] ? HIGH : LOW);
  }
  
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(ssid, password);
  client.begin(server, net);
  client.onMessage(messageReceived);

  connect();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
}

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

  client.subscribe(subscribeTopic);
}

void loop() 
{
  client.loop();
  delay(10);

  if(!client.connected()) 
  {
    connect();
  }

  httpServer.handleClient();

}

void messageReceived(String &topic, String &payload) 
{
  String msgString = payload;

  // Relay 1
  if (msgString == "R1ON")
  {
    relayStates[0] = true;
    digitalWrite(relays[0], HIGH);
  }
  else if (msgString == "R1OFF")
  {
    relayStates[0] = false;
    digitalWrite(relays[0], LOW);
  }
  // Relay 2  
  else if (msgString == "R2ON")
  {
    relayStates[1] = true;
    digitalWrite(relays[1], HIGH);
  }
  else if (msgString == "R2OFF")
  {
    relayStates[1] = false;
    digitalWrite(relays[1], LOW);
  }
  // Relay 3
  else if (msgString == "R3ON")
  {
    relayStates[2] = true;
    digitalWrite(relays[2], HIGH);
  }
  else if (msgString == "R3OFF")
  {
    relayStates[2] = false;
    digitalWrite(relays[2], LOW);
  }
  // Relay 4
  else if (msgString == "R4ON")
  {
    relayStates[3] = true;
    digitalWrite(relays[3], HIGH);
  }
  else if (msgString == "R4OFF")
  {
    relayStates[3] = false;
    digitalWrite(relays[3], LOW);
  }

  
  //Serial.write(0xA0);
  //Serial.write(0x04);
  
//   if(channel1 && channel2)
//   {
//     Serial.write(0x03); 
//   }
//   else if(channel1 && channel2 == false)
//   {
//     Serial.write(0x01); 
//   }
//   else if(channel2 && channel1 == false)
//   {
//     Serial.write(0x02); 
//   }
//   else
//   {
//     Serial.write(0x00); 
//   }
  
//   Serial.write(0xA1);
//   Serial.flush();
//   delay(250);
}