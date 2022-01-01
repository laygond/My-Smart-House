/*
 * < LAYGOND-AI >
 * Device Title: LAI-InfraredAirConditioner
 * Device Description: MQTT Infrared Transmitter to control AC unit 
 * Device Explanation: The device recieves an MQTT message from the server to
 *                     transmit an IR signal to the AC unit. These IR messages have 
 *                     been previously decoded
 *                     
 * Author: Bryan Laygond
 * Github: @laygond
 * 
 * Inspired by: 
 * Matt Kaczynski http://www.MK-SmartHouse.com 
 * Bill https://dronebotworkshop.com/
 * 
 * COPYRIGHT:
 * Code may only be distributed through https://github.com/laygond any 
 * other methods of obtaining or distributing are prohibited.
 * < LAYGOND-AI > Copyright (c) 2020
 * 
 * PREREQUISITES:
 * In Arduino IDE Install the following:
 * Under 'Sketch/Include Library/Manage Libraries'
 * - ArduinoJson by Benoit Blanchon      Version 5.13.5 (V5 is a must)
 * - MQTT        by Joel Gaehwiler       Version 2.4.7
 * - WiFiManager by tzapu                Version 2.0.3  
 * - IRRemote    by Armin Joachinsmeyer  Version 3.3
 * 
 * Under 'File/Preferences/Additional Board Manage URL' include
 * - http://arduino.esp8266.com/stable/package_esp8266com_index.json
 * 
 * Under 'Tools/Boards/Board Manager'
 * - esp8266     by ESP8266 Community Version 2.5.0
 * 
 * REMOTE ACCESS:
 * Once connected to your network you can access your ESP8266 ESP-12F by going to
 * http://HOSTNAMEOFDEVICE.local or http://YOUR_DEVICE_IP. Update of firmware can 
 * also be done remotely(manually) at http://YOUR_DEVICE_IP/firmware 
 * 
 * HARDWARE:
 * - 
 * - HX-53 Infrared Transmitter
 */


/* IRremoteESP8266: IRsendDemo - demonstrates sending IR codes with IRsend.
 *
 * Version 1.1 January, 2019
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 *
 * An IR LED circuit *MUST* be connected to the ESP8266 on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_LG.h>

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.
uint64_t data=0x5b84790b ;


void setup() {
  irsend.begin();
//#if ESP8266
//  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
//#else  // ESP8266
//  Serial.begin(115200, SERIAL_8N1);
//#endif  // ESP8266
}

void loop() {
  //Serial.println("LG2");
  irsend.sendLG2(data, 28, 1);  // 28 bits & 1 repeats

  delay(3000);
}
