# My-Smart-House
House Automation through OpenHab2

# Directory Structure
```
.My-Smart-House
├── README.md
├── firmware                 # IoT Device firmware
│   ├── LinkNode_R4-RelayBoard
│   ├── MK-DoorSensor
│   ├── MK-FireSensor
│   ├── MK-LEDStripController
│   └── wyze_v2
├── openhab2                 # soft link to openhab's directory
│   ├── items
│   ├── rules
│   ├── sitemaps
│   ├── things
│   └── ...
├── no_signal.png
└── simple_cam_monitor.py
```
# References

### Hardware
[MK-DoorSensor](https://www.MK-SmartHouse.com/door-sensor)
[LinkNode R4 relay board](https://www.linksprite.com/wiki/index.php?title=LinkNode_R4:_Arduino-compatible_WiFi_relay_controller)
[Gosund WP3 Smart Plug ](https://vmallet.com/2020/07/gosund-wp3-smart-plug-teardown-and-schematic/)

### Camera Protocol
[ONVIF Vs RTSP](https://ipvm.com/forums/video-surveillance/topics/very-confused-about-onvif-rtsp)
[RTSP Wyzecam](https://www.youtube.com/watch?v=8JeMudwlOzM)
[RTSP + App Wyzecam](https://www.youtube.com/watch?v=e0SgzWwt7yI)

### Load Code to IoT Devices
[Install Arduino in Linux](https://www.arduino.cc/en/guide/linux)
[Add ESP8266 to Arduino Board Manager](https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/)
[Discussion of QIO DIO QOUT DOUT flash modes](https://www.esp32.com/viewtopic.php?t=1250)
[Flash Firmware to Tuya Devices Over the Air](https://www.youtube.com/watch?v=O5GYh470m5k&ab_channel=digiblurDIY)
[Figuring out generic tuya devices control pins](https://www.youtube.com/watch?v=m_O24tTzv8g&ab_channel=digiblurDIY)
