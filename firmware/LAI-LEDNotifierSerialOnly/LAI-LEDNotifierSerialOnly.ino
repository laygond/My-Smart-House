// LED NOTIFICATION STRIP
// Core code of LAI-LEDNotifier with serial interface for debugging purposes
// by Laygond

#include <FastLED.h>

#define DATA_PIN    D4  
#define NUM_LEDS    50
#define BRIGHTNESS  100
#define LED_TYPE    WS2813
#define COLOR_ORDER RGB
#define UPDATES_PER_SECOND 100
CRGB leds[NUM_LEDS];
CRGB old_leds[NUM_LEDS];  //Used for storage
int sections[][3] ={148,0,211,    //Door Open
                    255,0,0,      //Hot(stove, oven, iron, etc)
                    0,255,0,      //Earthquake
                    255,255,0,    //Mailbox
                    0,255,255,    //New device in network
                    255,255,255,  //Gas or Smoke
                    0,0,255,      //Someone at door
                    255,140,0};  //No internet
int number_of_sections = sizeof(sections)/sizeof(*sections);
int number_of_LEDs_per_section = int(NUM_LEDS / number_of_sections);
int section_cmd;     // section command: 1 = ON, 0 = OFF
int section_idx;     // section index
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete


/**
*   Fill all color sections (used for debugging)
*/
void color_all_sections(){
  for(size_t i =0; i<NUM_LEDS; ++i){
    leds[i] = CRGB(0,0,0);
  }
  FastLED.show(); 
  for(size_t i =0; i<number_of_sections; ++i){
    for(size_t j =0; j<number_of_LEDs_per_section; ++j){
        leds[number_of_LEDs_per_section*i+j] = CRGB(sections[i][0],sections[i][1],sections[i][2]);
    }
  }
  FastLED.show();
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
* Reads string from serial monitor (used for debugging)
*/
void readStringFromSerialMonitor() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

/**
* Copy, transfer, or reassign LED Arrays (utility tool)
*/
void copyArray(CRGB* src, CRGB* dst, int len) {
    memcpy(dst, src, sizeof(src[0])*len);
}

void setup() {
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  Serial.begin(9600);
  inputString.reserve(20);   // reserve 20 bytes for inputString
}

void loop() {
  readStringFromSerialMonitor(); //String arrives in this format 1S6 = ON section 6, 0S6 = OFF section 6
  if (stringComplete) {
    section_cmd = inputString.substring(0,1).toInt();
    section_idx = inputString.substring(2).toInt();
    Serial.println(section_cmd);
    Serial.println(section_idx);
    inputString = "";
    stringComplete = false;
    if (section_cmd==1){//then Alert before triggering
      copyArray(leds, old_leds, NUM_LEDS); //save before changes
      alert_section(section_idx);        
      copyArray(old_leds, leds, NUM_LEDS); //recover 
    }
    trigger_section(section_idx, section_cmd);  
  }
}
