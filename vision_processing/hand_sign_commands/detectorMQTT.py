# Send MQTT command message to server based on detected hand sign
# Detection is done in a two step processL localization and then recognition
# by Laygond http://www.laygond.com

import cv2
from laygond import HandDetector, aspectRationize
from cvzone.ClassificationModule import Classifier
import numpy as np
import math
import time
import paho.mqtt.client as mqtt 

cap = cv2.VideoCapture(0)

# Detectors
detector = HandDetector(maxHands=1, minTrackCon=0.6)      # Used only as a localizer since lm are huge when hand is far from webcam
fine_detector = HandDetector(maxHands=1, minTrackCon=0.6) # Places lm and connections for only region of interest

# Classifier Parameters
classifier = Classifier("model/keras_model.h5", "model/labels.txt")
offset = 15             # Pre-Process: set an offset in case bounding box too close to hand
imgSize = 300           # Pre-Process: target size with which images will be/were trained
labels = ["JogaBonito","Metal"]

# MQTT Parameters
mqttBroker ="192.168.68.50" 
client = mqtt.Client("PyMQ")
client.connect(mqttBroker)
topic_tv = "brealm/tv"
topic_ac = "brealm/ac"
cmd_tv_on_off = '1xx'   #[POWER 1][VOLUME: 1-0 UP-DOWN)][CHANNEL: 1-0 UP-DOWN)]
cmd_ac_on  = '1135XX'   #[SECTION 1-2][POWER 1-0][TEMPERATURE: hex(temp-15)][FANSPEED: 0,9,2,A,4,5][SWING VERTICAL 1-0][SWING HORIZONTAL 1-0]
cmd_ac_off = '10XXXX'

# Run
while True:
    success, img = cap.read()

    # Localize all hands
    hands = detector.findHands(img, draw =False)
    if hands:

        # Preprocess Region of Interest(hand): resize and draw lm connections
        hand = hands[0]
        x, y, w, h = hand['bbox']
        roi = img[max(0,y-offset) : y+h+offset, max(0,x-offset) : x+w+offset]
        resized_roi = aspectRationize(roi, imgSize, colorFiller=(255,255,255))
        _ , resized_roi = fine_detector.findHands(resized_roi, draw_box=False)

        # Recognize hand sign and send MQTT command
        prediction, index = classifier.getPrediction(resized_roi, draw=False)
        print(prediction)
        # if labels[index]=="Metal":
        #     client.reconnect() # in case broker disconnects
        #     client.publish(topic_tv, cmd_tv_on_off)
        #     time.sleep(1.5) # TODO: can be improved through a debounce system like in hardware buttons
        # if labels[index]=="JogaBonito":
        #     client.reconnect() # in case broker disconnects
        #     client.publish(topic_ac, cmd_ac_on)
        #     time.sleep(.3) 

        # Draw and display Labels
        cv2.rectangle(img, (x - offset, y - offset-50),
                      (x - offset+90, y - offset-50+50), (255, 0, 255), cv2.FILLED)
        cv2.putText(img, labels[index], (x, y -26), cv2.FONT_HERSHEY_COMPLEX, 1.7, (255, 255, 255), 2)
        cv2.rectangle(img, (x-offset, y-offset),
                      (x + w+offset, y + h+offset), (255, 0, 255), 4)
        cv2.imshow("Region of Interest", roi)
        cv2.imshow("Corrected ROI", resized_roi)
 
    cv2.imshow("Image", img)
    cv2.waitKey(1)

            
       
 

