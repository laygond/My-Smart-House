/*
 * < LAYGOND-AI >
 * Device Title: LAI-InfraredTV
 * Device Description: MQTT Infrared Transmitter to control TV 
 * Device Explanation: The device recieves an MQTT message(string) from the server to
 *                     transmit an IR signal (uint32_t) to TV. These IR messages have 
 *                     been previously decoded using IRrecvDumpV2(example code from IRremoteESP8266)
 *                     
 * Author: Bryan Laygond
 * laygond.com
 * 
 * COPYRIGHT:
 * Code may only be distributed through https://github.com/laygond any 
 * other methods of obtaining or distributing are prohibited.
 * < LAYGOND-AI > Copyright (c) 2020
 * 
 * THEORY TO DECODE MQTT Message:
 * I have decided to create a string that stores the following information
 * [POWER 1][VOLUME: 1-0 UP-DOWN)][CHANNEL: 1-0 UP-DOWN)]
 * Example if you receive 1xx you turn on/off tv ignoring volume and channel
 * but if you receive x1x you turn up volume ignore everything else. You cannot
 * receive two or more commands in a single string, e.g. 110, only first part 
 * will be processed, i.e. power.
 * 
 * NOTE:
 * On a Arduino Uno/Nano with a 8-bit microcontroller, such as the ATMega328P, an int is a int16_t. 
 * On the 32-bit ESP8266 CPU, an int is int32_t.
 * 
 * HARDWARE: 
 * - ESP8266 d1 mini
 * - HX-53 Infrared Transmitter
 * - SAMSUNG TV which works with REMOTE BN5900849A 
 */

/* ---------- DO NOT EDIT ANYTHING IN THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING---------- */
#include <FS.h>                   //this needs to be first, or it all crashes and burns...lol
#include <IRremoteESP8266.h>      //IR needed libraries
#include <IRsend.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <MQTTClient.h>           //https://github.com/256dpi/arduino-mqtt
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

// TV Related Variables  (SAMSUNG REMOTE BN5900849A)   
#define TV_VOLUME_UP        0xE0E0E01F
#define TV_VOLUME_DOWN      0xE0E0D02F
#define TV_CHANNEL_DOWN     0xE0E008F7
#define TV_CHANNEL_UP       0xE0E048B7
#define TV_ON_OFF           0xE0E040BF
IRsend irsend(4);          // GPIO pin 4 (D2).

//Web GUI Parameter Variables
//if there are different values in config.json, they are overwritten.
char host[34];
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[50];
char mqtt_isAuthentication[7] = "FALSE";
char mqtt_username[40];
char mqtt_password[40];
char update_username[40];
char update_password[40];
char update_path[34] = "/firmware";

//Unique device ID 
const char* mqttDeviceID;

//Form Custom SSID
String ssidAP = "LAI-IRTV" + String(ESP.getChipId());

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () 
{
  shouldSaveConfig = true;
}

// Server and Client instances
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient net;
MQTTClient client;

//Forward Declarations 
void connect();
void messageReceived( String &topic, String &payload);


void setup() 
{
  if (SPIFFS.begin()) 
  {
    if (SPIFFS.exists("/config.json")) 
    {
      //file exists, reading and loading
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) 
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) 
        {
          strcpy(host, json["host"]);
          strcpy(update_username, json["update_username"]);
          strcpy(update_password, json["update_password"]);
          strcpy(mqtt_isAuthentication, json["mqtt_isAuthentication"]);
          strcpy(mqtt_username, json["mqtt_username"]);
          strcpy(mqtt_password, json["mqtt_password"]);
          strcpy(update_path, json["update_path"]);
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
        } 
      }
    }
  } 
  //end json read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_text0("<p>Select your wifi network and type in your password, if you do not see your wifi then scroll down to the bottom and press scan to check again.</p>");
  WiFiManagerParameter custom_text1("<h1>Hostname/MQTT ID</h1>");
  WiFiManagerParameter custom_text2("<p>Enter a name for this device which will be used for the hostname on your network and identify the device from MQTT.</p>");
  WiFiManagerParameter custom_host("name", "Device Name", host, 32);
  
  WiFiManagerParameter custom_text3("<h1>MQTT</h1>");
  WiFiManagerParameter custom_text4("<p>Enter the details of your MQTT server and then enter the topic for which the device listens to MQTT commands from. If your server requires authentication then set it to True and enter your server credentials otherwise leave it at false and keep the fields blank.</p>");
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server IP", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Server Port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", mqtt_topic, 50);
  WiFiManagerParameter custom_mqtt_isAuthentication("isAuthentication", "MQTT Authentication?", mqtt_isAuthentication, 7);
  WiFiManagerParameter custom_mqtt_username("userMQTT", "Username For MQTT Account", mqtt_username, 40);
  WiFiManagerParameter custom_mqtt_password("passwordMQTT", "Password For MQTT Account", mqtt_password, 40);

  WiFiManagerParameter custom_text5("<h1>Web Updater</h1>");
  WiFiManagerParameter custom_text6("<p>The web updater allows you to update the firmware of the device via a web browser by going to its ip address or hostname /firmware ex. 192.168.0.5/firmware you can change the update path below. The update page is protected so enter a username and password you would like to use to access it. </p>");
  WiFiManagerParameter custom_update_username("user", "Username For Web Updater", update_username, 40);
  WiFiManagerParameter custom_update_password("password", "Password For Web Updater", update_password, 40);
  WiFiManagerParameter custom_device_path("path", "Updater Path", update_path, 32);
  WiFiManagerParameter custom_text8("<p>*To reset device parameter settings click on RST on the ESP8266 for 10 seconds.*</p>");
  WiFiManagerParameter custom_text9("");

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.setCustomHeadElement("<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:oswald;} button{border:0;background-color:#313131;color:white;line-height:2.4rem;font-size:1.2rem;text-transform: uppercase;width:100%;font-family:oswald;} .q{float: right;width: 65px;text-align: right;} body{background-color: #575757;}h1 {color: white; font-family: oswald;}p {color: white; font-family: open+sans;}a {color: #78C5EF; text-align: center;line-height:2.4rem;font-size:1.2rem;font-family:oswald;}</style>");
  wifiManager.addParameter(&custom_text0);
  wifiManager.addParameter(&custom_text1);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_host);
  
  wifiManager.addParameter(&custom_text3);
  wifiManager.addParameter(&custom_text4);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_mqtt_isAuthentication);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);

  wifiManager.addParameter(&custom_text5);
  wifiManager.addParameter(&custom_text6);
  wifiManager.addParameter(&custom_update_username);
  wifiManager.addParameter(&custom_update_password);
  wifiManager.addParameter(&custom_device_path);
  wifiManager.addParameter(&custom_text8);
  
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(240); //give user 4 minutes max to enter credentials

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(ssidAP.c_str())) 
  {
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //read updated parameters
  strcpy(host, custom_host.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  strcpy(mqtt_isAuthentication, custom_mqtt_isAuthentication.getValue());
  strcpy(mqtt_username, custom_mqtt_username.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());
  strcpy(update_username, custom_update_username.getValue());
  strcpy(update_password, custom_update_password.getValue());
  strcpy(update_path, custom_device_path.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) 
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["host"] = host;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;
    json["mqtt_isAuthentication"] = mqtt_isAuthentication;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["update_username"] = update_username;
    json["update_password"] = update_password;
    json["update_path"] = update_path;

    File configFile = SPIFFS.open("/config.json", "w");
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  delay(5000);

  // Setup IR to send Command Signal
  irsend.begin();
  
  // Reset ESP if missing or wrong web gui parameters
  if(String(host).length() == 0 || String(mqtt_server).length() == 0 || String(mqtt_topic).length() == 0 || String(update_username).length() == 0 || String(update_password).length() == 0)
  {
    wifiManager.resetSettings();
    ESP.reset();
  }

  // Connect to everything
  mqttDeviceID = host;
  client.begin(mqtt_server, atoi(mqtt_port), net);
  client.onMessage(messageReceived);
  connect();

 // ESP IP Web address after connection 
  MDNS.begin(host);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.on("/", [](){
    if(!httpServer.authenticate(update_username, update_password))
      return httpServer.requestAuthentication();
    httpServer.send(200, "text/plain", "< LAYGOND-AI > \nHostname: " + String(host) + "\nMQTT Server: " + String(mqtt_server) + "\nMQTT Port: " + String(mqtt_port) + "\nMQTT Authentication: " + String(mqtt_isAuthentication) + "\nMQTT Command Topic: " + String(mqtt_topic) + "\nMQTT Status Topic: " + String(mqtt_topic) + "/state" + "\nTo update firmware go to: http://"+ String(host) + ".local" + String(update_path) + "\n*To reset device parameter settings click on RST on the ESP8266 for 10 seconds.*");
  });
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
}

void loop() 
{
  // Check Connection and Handle Client
  client.loop();
  delay(10);
  if(!client.connected()){
      client.disconnect();
      connect();}
  httpServer.handleClient();

 // === MESSAGE SEND (PUBLISH TO MQTT SERVER)===
 // In this case it only publishes on connect. It does not monitor anything through a sensor
 // to send back to MQTT server
}

/**
 * Deals with routines on start/connect: WiFi, MQTT, Reread sensors,etc
 */
void connect()
{
  //Connect to WiFi
  while (WiFi.status() != WL_CONNECTED){
      delay(1000);}
  WiFi.mode(WIFI_STA);      //prevents broadcasting AP after connection
  
  //Connect to MQTT
  if(String(mqtt_isAuthentication).equalsIgnoreCase("TRUE")){
      while (!client.connect(mqttDeviceID, mqtt_username, mqtt_password)){
          delay(1000);}}
  else{
      while (!client.connect(mqttDeviceID)){
          delay(1000);}}
  
  // Subscribe to topic
  client.subscribe(mqtt_topic);
  
  // === OPTIONAL ===
  //Reread sensor variables and/or publish to MQTT Server before it goes into the loop() logic
  delay(100);
  //In my case i just want to let the server know there was a disconnection
  //so that it sends back all the event states if needed
  client.publish(String(mqtt_topic)+ "/state", "CONNECTED");  
  delay(50);
}

/**
 * Decode MQTT message to send corrensponding IR signal to TV
 */
void messageReceived( String &topic, String &payload)
{
  String msgString = payload;
  //String arrives in this format (without the brackets):
  //[POWER 1][VOLUME: 1-0 UP-DOWN)][CHANNEL: 1-0 UP-DOWN)]
  //Example if you receive 1xx you turn on/off tv ignoring volume and channel
  //                       x1x you turn up volume ignore everything else
  
  if (isValidNumber(msgString.substring(0,1))){
      int power = msgString.substring(0,1).toInt();
      if (power == 1){
          irsend.sendSAMSUNG(TV_ON_OFF);}}
  
  else if (isValidNumber(msgString.substring(1,2))){
      int volume = msgString.substring(1,2).toInt();
      if (volume == 0){ 
          irsend.sendSAMSUNG(TV_VOLUME_DOWN);}
      if (volume == 1){
          irsend.sendSAMSUNG(TV_VOLUME_UP);}}

  else if (isValidNumber(msgString.substring(2,3))){
      int channelTV = msgString.substring(2,3).toInt();
      if (channelTV == 0){ 
          irsend.sendSAMSUNG(TV_CHANNEL_DOWN);}
      if (channelTV == 1){
          irsend.sendSAMSUNG(TV_CHANNEL_UP);}}
 }


/**
 * Checks whether string is a number (Helper Func)
 */
boolean isValidNumber(String str)
{
   boolean isNum=false;
   if(!(str.charAt(0) == '+' || str.charAt(0) == '-' || isDigit(str.charAt(0)))) return false;

   for(byte i=1;i<str.length();i++)
   {
       if(!(isDigit(str.charAt(i)) || str.charAt(i) == '.')) return false;
   }
   return true;
}
