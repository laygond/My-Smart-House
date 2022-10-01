# Saves hand images by pressing letter 'S' on Keyboard
#
# by Laygond http://www.laygond.com
# inspired by Murtaza  https://www.youtube.com/watch?v=wa2ARoUUdU8&t=145s&ab_channel=Murtaza%27sWorkshop-RoboticsandAI
#
# NOTE: Set 'folder' path variable in code before continuing  


import cv2
from laygond import HandDetector, aspectRationize
import numpy as np
import math
import time
 
cap = cv2.VideoCapture(0)
detector = HandDetector(maxHands=1, minTrackCon=0.6)
fine_detector = HandDetector(maxHands=1, minTrackCon=0.6) # Added since lm are huge when hand is far from webcam
 
offset = 15             # Pre-Process: set an offset in case bounding box too close to hand
imgSize = 300           # Pre-Process: target size with which images will be/were trained
folder = "data/jogabonito"   # SET YOUR OWN PATH (For Each Hand Sign)
counter = 0             # Just a counter to visualize number of collected images
 
while True:
    success, img = cap.read()
    hands = detector.findHands(img, draw =False)
    if hands:
        hand = hands[0]
        x, y, w, h = hand['bbox']

        roi = img[max(0,y-offset) : y+h+offset, max(0,x-offset) : x+w+offset]
        resized_roi = aspectRationize(roi, imgSize, colorFiller=(255,255,255))
        _ , resized_roi = fine_detector.findHands(resized_roi, draw_box=False)
       
        cv2.imshow("Region of Interest", roi)
        cv2.imshow("Corrected ROI", resized_roi)
 
    cv2.imshow("Image", img)
    key = cv2.waitKey(1)
    if key == ord("s"): # s of save
        counter += 1
        cv2.imwrite(f'{folder}/image_{time.time()}.jpg',resized_roi)
        print(counter)
