/*
 * Device Title: MK-LEDStripControl V2
 * Device Description: MQTT LED Strip Control
 * Device Explanation: The device recieves an MQTT message from the server and
 *                     changes the color of the LED Strip
 * Device information: https://www.MK-SmartHouse.com/led-control
 *                     
 * Author: Matt Kaczynski
 * Website: http://www.MK-SmartHouse.com
 * 
 * Code may only be distrbuted through http://www.MK-SmartHouse.com any other methods
 * of obtaining or distributing are prohibited
 * Copyright (c) 2016-2018 
 * 
 * Note: After flashing the code once you can remotely access your device by going to http://HOSTNAMEOFDEVICE.local/firmware 
 * obviously replace HOSTNAMEOFDEVICE with whatever you defined below. The user name and password are also defined below.
 */
 
/* ---------- DO NOT EDIT ANYTHING IN THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING---------- */
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <Servo.h> 
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <MQTTClient.h>           //https://github.com/256dpi/arduino-mqtt
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

//define your default values here, if there are different values in config.json, they are overwritten.
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
char blinds_goSlow[7] = "FALSE";

//Unique device ID 
const char* mqttDeviceID;

//Form Custom SSID
String ssidAP = "MK-LEDStripControl" + String(ESP.getChipId());

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () 
{
  shouldSaveConfig = true;
}

// Declare Esp8266 Pins Used For LED Strip
const int greenLed = 14;
const int redLed = 12;
const int blueLed = 13;

// initialize led values
String greenLedValS = "0";
String redLedValS = "0";
String blueLedValS = "0";

// initialize led values
int greenLedVal = 0;
int redLedVal = 0;
int blueLedVal = 0;

int fader = 0;
int warmFader = 0;
int coldFader = 0;

// initialize led values
int greenLedValTemp = 0;
int redLedValTemp = 0;
int blueLedValTemp = 0;

// initialize led values
String greenLedValLast = "0";
String redLedValLast = "0";
String blueLedValLast = "0";

int lowestNumber;
int aSpeed = 100;
//effect Variable
enum Effects {NONE, SETCOLOR, RAINBOW, BREATHIN, BREATHOUT, POLICERED, POLICEBLUE, FADE, WARM, COLD};
enum Rainbow {REDD, ORANGE, YELOW, GREN, BLU, INDIGO, VIOLET};
enum Fader {FADE1, FADE2, FADE3, FADE4, FADE5, FADE6};
enum WC {UP, DOWN};
WC currentWarm = UP;
WC currentCold = UP;
Fader currentFader = FADE1;
Rainbow currentRainbow = REDD;
Effects currentEffect = NONE; 
Effects originalEffect = NONE; 

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

Servo myservo;

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect();

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
        else 
        {
        }
      }
    }
  } 
  else 
  {
  }
  //end read

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
  WiFiManagerParameter custom_text7("<p>*To reset device settings restart the device and quickly move the jumper from RUN to PGM, wait 10 seconds and put the jumper back to RUN.*</p>");
  WiFiManagerParameter custom_text8("");

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
  wifiManager.addParameter(&custom_text7);
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
    if (!configFile) 
    {
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  delay(5000);
  
  if(digitalRead(0) == LOW || String(host).length() == 0 || String(mqtt_server).length() == 0 || String(mqtt_topic).length() == 0 || String(update_username).length() == 0 || String(update_password).length() == 0)
  {
    wifiManager.resetSettings();
    ESP.reset();
  }
  
  mqttDeviceID = host;
  client.begin(mqtt_server, atoi(mqtt_port), net);
  client.onMessage(messageReceived);

  connect();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  httpServer.on("/", [](){
    if(!httpServer.authenticate(update_username, update_password))
      return httpServer.requestAuthentication();
    httpServer.send(200, "text/plain", "Hostname: " + String(host) + "\nMQTT Server: " + String(mqtt_server) + "\nMQTT Port: " + String(mqtt_port) + "\nMQTT Authentication: " + String(mqtt_isAuthentication) + "\nMQTT Command Topic: " + String(mqtt_topic) + "\nMQTT Status Topic: " + String(mqtt_topic) + "/state" + "\nTo update firmware go to: http://"+ String(host) + ".local" + String(update_path) + "\n*To reset device settings restart the device and quickly move the jumper from RUN to PGM, wait 10 seconds and put the jumper back to RUN.*");
  });
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);

}

void connect() 
{
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
  }

  //If authentication true then connect with username and password
  if(String(mqtt_isAuthentication).equalsIgnoreCase("TRUE"))
  {
    while (!client.connect(mqttDeviceID, mqtt_username, mqtt_password)) 
    {
      delay(1000);
    }
  }
  else
  {
    while (!client.connect(mqttDeviceID)) 
    {
      delay(1000);
    }
  }

  client.subscribe(mqtt_topic);
  client.publish(String(mqtt_topic) + "/state", "Start");
  client.publish(String(mqtt_topic) + "/state/speed", String(aSpeed));
  client.publish(String(mqtt_topic) + "/state/effect", "NONE");
}

void loop() 
{
  client.loop();
  delay(10);

  if(!client.connected()) 
  {
    //connect();
    client.disconnect();
    connect();
  }

  switch (currentEffect)
  {
    case NONE:
      break;
    case SETCOLOR:
      analogWrite(blueLed, blueLedVal);
      analogWrite(redLed, redLedVal);
      analogWrite(greenLed, greenLedVal);
      currentEffect = originalEffect;
      break;
    case RAINBOW:
      switch(currentRainbow)
      {
        case REDD:
          greenLedVal = 0;
          redLedVal = 1023;
          blueLedVal = 0;
          currentRainbow = YELOW;
          break;
        case ORANGE:
          greenLedVal = 700;
          redLedVal = 1023;
          blueLedVal = 0;
          currentRainbow = YELOW;
          break;
        case YELOW:
          greenLedVal = 1023;
          redLedVal = 1023;
          blueLedVal = 0;
          currentRainbow = GREN;
          break; 
        case GREN:
          greenLedVal = 1023;
          redLedVal = 0;
          blueLedVal = 0;
          currentRainbow = BLU;
          break; 
        case BLU:
          greenLedVal = 0;
          redLedVal = 0;
          blueLedVal = 1023;
          currentRainbow = INDIGO;
          break; 
        case INDIGO:
          greenLedVal = 0;
          redLedVal = 1023;
          blueLedVal = 1023;
          currentRainbow = VIOLET;
          break; 
        case VIOLET:
          greenLedVal = 100;
          redLedVal = 1023;
          blueLedVal = 588;
          currentRainbow = REDD;
          break;
      }
      greenLedValTemp = 0;
      redLedValTemp = 0;
      blueLedValTemp = 0;
      
      currentEffect = BREATHOUT;
      break;
    case BREATHIN:
      currentEffect = BREATHOUT;
      if(blueLedValTemp > (blueLedVal/100))
      {
        analogWrite(blueLed, blueLedValTemp -= (blueLedVal/100));
        currentEffect = BREATHIN;
      }
      if(redLedValTemp > (redLedVal/100))
      {
        analogWrite(redLed, redLedValTemp -= (redLedVal/100));
        currentEffect = BREATHIN;
      }
      if(greenLedValTemp > (greenLedVal/100))
      {
        analogWrite(greenLed, greenLedValTemp -= (greenLedVal/100));
        currentEffect = BREATHIN;
      }
      if(currentEffect == BREATHOUT && originalEffect == RAINBOW)
      {
        currentEffect = RAINBOW;
        //client.publish(String(mqtt_topic) + "/state", "RAINBOW");
      }
      delay(100 - aSpeed); 
      break;
    case BREATHOUT:
      currentEffect = BREATHIN;
      if(blueLedValTemp < blueLedVal)
      {
        analogWrite(blueLed, blueLedValTemp += (blueLedVal/100));
        currentEffect = BREATHOUT;
      }
      if(redLedValTemp < redLedVal)
      {
        analogWrite(redLed, redLedValTemp += (redLedVal/100));
        currentEffect = BREATHOUT;
      }
      if(greenLedValTemp < greenLedVal)
      {
        analogWrite(greenLed, greenLedValTemp += (greenLedVal/100));
        currentEffect = BREATHOUT;
      }
      delay(100 - aSpeed); 
      break;
    case FADE:
      switch(currentFader)
      {
        case FADE1:
          analogWrite(redLed, 1020);
          analogWrite(greenLed, fader);
          analogWrite(blueLed, 0);
          fader += 10;
          if(fader == 1020)
          {
            //fader = 0;
            currentFader = FADE2;
          }
          break;
        case FADE2:
          analogWrite(redLed, fader);
          analogWrite(greenLed, 1020);
          analogWrite(blueLed, 0);
          fader -= 10;
          if(fader == 0)
          {
            //fader = 0;
            currentFader = FADE3;
          }
          break;
        case FADE3:
          analogWrite(redLed, 0);
          analogWrite(greenLed, 1020);
          analogWrite(blueLed, fader);
          fader += 10;
          if(fader == 1020)
          {
            //fader = 0;
            currentFader = FADE4;
          }
          break;
        case FADE4:
          analogWrite(redLed, 0);
          analogWrite(greenLed, fader);
          analogWrite(blueLed, 1020);
          fader -= 10;
          if(fader == 0)
          {
            //fader = 0;
            currentFader = FADE5;
          }
          break;
        case FADE5:
          analogWrite(redLed, fader);
          analogWrite(greenLed, 0);
          analogWrite(blueLed, 1020);
          fader += 10;
          if(fader == 1020)
          {
            //fader = 0;
            currentFader = FADE6;
          }
          break;
        case FADE6:
          analogWrite(redLed, 1020);
          analogWrite(greenLed, 0);
          analogWrite(blueLed, fader);
          fader -= 10;
          if(fader == 0)
          {
            //fader = 0;
            currentFader = FADE1;
          }
          break;
      }
      delay(100 - aSpeed);
      break;
    case POLICERED:
      analogWrite(blueLed, 0);
      analogWrite(redLed, 1023);
      analogWrite(greenLed, 0);
      currentEffect = POLICEBLUE;
      delay(125 - aSpeed); 
      break;
    case POLICEBLUE:
      analogWrite(blueLed, 1023);
      analogWrite(redLed, 0);
      analogWrite(greenLed, 0);
      currentEffect = POLICERED;
      delay(125 - aSpeed);
      break;
    case WARM:
      switch(currentWarm)
      {
        case UP:
          if(warmFader == 515)
            currentWarm = DOWN;
          else
            warmFader += 5;
          break;
        case DOWN:
          if(warmFader == 0)
            currentWarm = UP;
          else
            warmFader -= 5;
          break;
      }
      analogWrite(redLed, 1023);
      analogWrite(greenLed, warmFader);
      analogWrite(blueLed, 0);
      delay(125 - aSpeed);
      break;
      
    case COLD:
      switch(currentCold)
      {
        case UP:
          if(coldFader == 1015)
            currentCold = DOWN;
          else
            coldFader += 5;
          break;
        case DOWN:
          if(coldFader == 0)
            currentCold= UP;
          else
            coldFader -= 5;
          break;
      }
      analogWrite(redLed, 0);
      analogWrite(greenLed, coldFader);
      analogWrite(blueLed, 1023);
      delay(100-aSpeed);
      break;
    default:
    // statements
    break;
  }

  httpServer.handleClient();
}

void messageReceived(String &topic, String &payload) 
{
  String stringOne = payload;

  // find first ; in the string
  int firstClosingBracket = stringOne.indexOf(';')+1;

  if(firstClosingBracket > 1)
  {
    // find second ; in the string
    int secondOpeningBracket = firstClosingBracket + 1;
    int secondClosingBracket = stringOne.indexOf(';', secondOpeningBracket);
  
    // find the third ; in the string
    int thirdOpeningBracket = secondClosingBracket + 1;
    int thirdClosingBracket = stringOne.indexOf(';', thirdOpeningBracket);
  
    // using the locations of ; find values 
    redLedValS= stringOne.substring(0 , (firstClosingBracket - 1));
    greenLedValS = stringOne.substring(firstClosingBracket , secondClosingBracket);
    blueLedValS = stringOne.substring((secondClosingBracket +1) , thirdClosingBracket);
  
    if ((blueLedValS != blueLedValLast) || (greenLedValS != greenLedValLast) || (redLedValS != redLedValLast))
    {
      blueLedVal = blueLedValS.toInt() * 10.23;
      redLedVal = redLedValS.toInt() * 10.23;
      greenLedVal = greenLedValS.toInt() * 10.23;

      // initialize led values
      greenLedValTemp = greenLedVal;
      redLedValTemp = redLedVal;
      blueLedValTemp = blueLedVal;

      currentEffect = SETCOLOR;
      
      greenLedValLast = greenLedVal;
      redLedValLast = redLedVal;
      blueLedValLast = blueLedVal;
    }
    client.publish(String(mqtt_topic) + "/state/color", stringOne);
  }
  else if(isValidNumber(stringOne))
  {
    aSpeed = stringOne.toInt();
    client.publish(String(mqtt_topic) + "/state/speed", String(aSpeed));
  }
  else if(stringOne == "RAINBOW")
  {
    originalEffect = RAINBOW;
    currentEffect = RAINBOW;
    client.publish(String(mqtt_topic) + "/state/effect", "RAINBOW");
  }
  else if(stringOne == "BREATH")
  {
    originalEffect = BREATHIN;
    currentEffect = SETCOLOR;
    client.publish(String(mqtt_topic) + "/state/effect", "BREATH");
  }
  else if(stringOne == "POLICE")
  {
    originalEffect = POLICERED;
    currentEffect = SETCOLOR;
    client.publish(String(mqtt_topic) + "/state/effect", "POLICE");
  }
  else if(stringOne == "FADE")
  {
    originalEffect = FADE;
    currentEffect = SETCOLOR;
    client.publish(String(mqtt_topic) + "/state/effect", "FADE");
  }
  else if(stringOne == "WARM")
  {
    originalEffect = WARM;
    currentEffect = SETCOLOR;
    client.publish(String(mqtt_topic) + "/state/effect", "WARM");
  }
  else if(stringOne == "COLD")
  {
    originalEffect = COLD;
    currentEffect = SETCOLOR;
    client.publish(String(mqtt_topic) + "/state/effect", "COLD");
  }
  else if(stringOne == "NONE")
  {
    originalEffect = NONE;
    currentEffect = NONE;
    client.publish(String(mqtt_topic) + "/state/effect", "NONE");
  }
  client.publish(String(mqtt_topic) + "/state", stringOne);
}

int divisor()
{
  return min(min(blueLedVal, redLedVal), greenLedVal);  
}

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
