/*
 * < LAYGOND-AI >
 * Device Title: LAI-LEDNotifier
 * Device Description: Get Notifications as led color lighting via MQTT. 
 * 
 * Device Explanation: An LED strip is split into sections where each section represents the 
 *                     status of an event. Each section has an equal number of pixels and a 
 *                     corresponding color and position in the LED strip. When a section, 
 *                     e.g. 'section 0', lights up it may mean the door is open, the fire 
 *                     detector went off, there is mail in the mailbox, etc; your imagination 
 *                     is the limit. When an event is received, a color alert display is 
 *                     first shown followed by the lighting up of the section. On connection, 
 *                     this device sends a request to the mqtt server for event statuses in 
 *                     case the power went out.
 *                         
 * Author: Bryan Laygond
 * laygond.com
 *  
 * Code may only be distributed through https://github.com/laygond/ any 
 * other methods of obtaining or distributing are prohibited.
 * < LAYGOND-AI > Copyright (c) 2020
 *
 * TO DO:
 * - instead of old_leds just keep track of sections
 * - if led strip over 10 meters check this https://techtutorialsx.com/2018/02/13/esp32-arduino-variable-length-arrays/
 * 
 * HARDWARE:
 * - ESP8266 D1 Mini 
 * - WS2815 LED Strip (same as WS2813, WS2812)
 */
 
/* ---------- DO NOT EDIT ANYTHING IN THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING---------- */
#include <FS.h>                   //this needs to be first, or it all crashes and burns...lol
#include <FastLED.h> 
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <MQTTClient.h>           //https://github.com/256dpi/arduino-mqtt
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

//LED Related Variables
#define DATA_PIN    D4  
#define BRIGHTNESS  100
#define LED_TYPE    WS2813
#define COLOR_ORDER RGB
#define UPDATES_PER_SECOND 100        
#define MAX_LEDS_AND_SECTIONS 180      //Around 3 meters of LED strip(60 pixels/m)
CRGB leds[MAX_LEDS_AND_SECTIONS];
CRGB old_leds[MAX_LEDS_AND_SECTIONS];  //Used for storage
int NUM_LEDS;     //Obtained trough Web GUI through string num_leds
int sections[][3] ={148,0,211,    //Door Open
                    255,0,0,      //Hot(stove, oven, iron, etc)
                    0,255,0,      //Earthquake
                    255,255,0,    //Mailbox
                    0,255,255,    //New device in network
                    255,255,255,  //Gas or Smoke
                    0,0,255,      //Someone at door
                    255,140,0};  //No internet
int number_of_sections = sizeof(sections)/sizeof(*sections);
int number_of_LEDs_per_section;
int section_cmd;     // section command: 1 = ON, 0 = OFF
int section_idx;     // section index

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
char num_leds[5];

//Unique device ID 
const char* mqttDeviceID;

//Form Custom SSID
String ssidAP = "LAI-LED" + String(ESP.getChipId());

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
void trigger_section(int k, int value);
void alert_section(int k);
void copyArray(CRGB* src, CRGB* dst, int len);


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
          strcpy(num_leds, json["num_leds"]);
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

  WiFiManagerParameter custom_text10("<h1>LED</h1>");
  WiFiManagerParameter custom_text11("<p>Enter LED Parameters. Make sure there are enough pixels for all the events you plan on creating. Strip must be up to 180 LEDs.</p>");
  WiFiManagerParameter custom_num_leds("num_leds", "Number of pixels in LED Strip", num_leds, 5);
  
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

  wifiManager.addParameter(&custom_text10);
  wifiManager.addParameter(&custom_text11);
  wifiManager.addParameter(&custom_num_leds);
  
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
  wifiManager.setTimeout(120);

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
  strcpy(num_leds, custom_num_leds.getValue());

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
    json["num_leds"] = num_leds;

    File configFile = SPIFFS.open("/config.json", "w");
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  delay(5000);

  // Setup LED strip (Assign gui parameter to global variable)
  NUM_LEDS = atoi(num_leds);  
  number_of_LEDs_per_section = int(NUM_LEDS / number_of_sections);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  
  // Reset ESP if missing or wrong web gui parameters
  if(digitalRead(0) == LOW || String(num_leds).length() == 0 || NUM_LEDS > MAX_LEDS_AND_SECTIONS || String(host).length() == 0 || String(mqtt_server).length() == 0 || String(mqtt_topic).length() == 0 || String(update_username).length() == 0 || String(update_password).length() == 0)
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
    httpServer.send(200, "text/plain", "< LAYGOND-AI > \nHostname: " + String(host) + "\nMQTT Server: " + String(mqtt_server) + "\nMQTT Port: " + String(mqtt_port) + "\nMQTT Authentication: " + String(mqtt_isAuthentication) + "\nMQTT Command Topic: " + String(mqtt_topic) + "\nMQTT Status Topic: " + String(mqtt_topic) + "/state" + "\nTo update firmware go to: http://"+ String(host) + ".local" + String(update_path) + "\nNumber of LEDS: " + String(num_leds) + "\n*To reset device parameter settings click on RST on the ESP8266 for 10 seconds.*");
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
  delay(10000);
  //In my case i just want to let the server know there was a disconnection
  //so that it sends back all the event states 
  client.publish(String(mqtt_topic)+ "/state", "CONNECTED");  
  delay(50);
}

/**
 * Alert and trigger color section in LED strip when receiving MQTT message
 */
void messageReceived( String &topic, String &payload)
{
  String msgString = payload;
  //String arrives in this format 1S6 = ON section 6, 0S6 = OFF section 6
  section_cmd = msgString.substring(0,1).toInt();
  section_idx = msgString.substring(2).toInt();
  if (section_cmd==1){//then Alert before triggering
    copyArray(leds, old_leds, NUM_LEDS); //save before changes
    alert_section(section_idx);        
    copyArray(old_leds, leds, NUM_LEDS); //recover 
  }
  trigger_section(section_idx, section_cmd);  
}

/**
*   Turn ON or OFF a color section
*/
void trigger_section(int k, int value){
  if (k < number_of_sections) {
    if(value==1){ 
      for(size_t j =0; j<number_of_LEDs_per_section; ++j){
          leds[number_of_LEDs_per_section*k+j] = CRGB(sections[k][0],sections[k][1],sections[k][2]);
      }
    }
    if(value==0){ 
      for(size_t j =0; j<number_of_LEDs_per_section; ++j){
          leds[number_of_LEDs_per_section*k+j] = CRGB(0,0,0);
      }
    }
    FastLED.show();
  }
}

/**                   
 * Catch visual attention with incoming section k color                    
 */
void alert_section(int k){
  if (k < number_of_sections) {
    // Reset pixels
    for(size_t i =0; i<NUM_LEDS; ++i){
      leds[i] = CRGB(0,0,0);
    }
    FastLED.show();
    // Increase from ends towards center
    for(size_t i =0; i<NUM_LEDS/2; ++i){
      leds[i] = CRGB(sections[k][0],sections[k][1],sections[k][2]);
      leds[NUM_LEDS-1-i] = CRGB(sections[k][0],sections[k][1],sections[k][2]);
      delay(10);
      FastLED.show();
    }
    // Decrease from center toward ends
    for(int i =int(NUM_LEDS/2); i>=0; --i){
      leds[i] = CRGB(0,0,0);
      leds[NUM_LEDS-1-i] = CRGB(0,0,0);
      delay(10);
      FastLED.show();
    }
  }
}

/**
* Copy, transfer, or reassign LED Arrays (utility tool)
*/
void copyArray(CRGB* src, CRGB* dst, int len) {
    memcpy(dst, src, sizeof(src[0])*len);
}
