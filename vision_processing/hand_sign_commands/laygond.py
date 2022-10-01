"""
Hand & Pose Tracking Module
By: LAYGOND-AI
Website: https://www.laygond.com

Inspired by Murtaza and Adrian Rosebrock 
"""

import cv2
import mediapipe as mp
import math
import numpy as np


class HandDetector:
    """
    Finds Hands using the mediapipe library.
    - Exports landmarks(lm) in pixel format
    - Provides bounding box(bb) info of hands
    - Draws lm connections and bb
    """

    def __init__(self, mode=False, maxHands=2, detectionCon=0.5, minTrackCon=0.5):
        """
        :param mode: In static mode, detection is done on each image: slower
        :param maxHands: Maximum number of hands to detect
        :param detectionCon: Minimum Detection Confidence Threshold
        :param minTrackCon: Minimum Tracking Confidence Threshold
        """
        self.mode = mode
        self.maxHands = maxHands
        self.detectionCon = detectionCon
        self.minTrackCon = minTrackCon

        self.mpHands = mp.solutions.hands
        self.hands = self.mpHands.Hands(static_image_mode=self.mode, max_num_hands=self.maxHands,
                                        min_detection_confidence=self.detectionCon,
                                        min_tracking_confidence=self.minTrackCon)
        self.mpDraw = mp.solutions.drawing_utils
        self.tipIds = [4, 8, 12, 16, 20]
        self.fingers = []
        self.lmList = []

    def findHands(self, img, draw=True, draw_box=True, draw_connections=True, flipType=True):
        """
        Finds hands in a BGR image.
        :param img: Image to find the hands in.
        :param draw_box: Flag to turn off all drawings if False.
        :param draw_box: Flag to draw box on output image.
        :param draw_box: Flag to draw landmark connections on output image.
        :return: Image with or without drawings
        """
        if draw is False:
            draw_box=False
            draw_connections=False

        imgRGB = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        self.results = self.hands.process(imgRGB)
        allHands = []
        h, w, c = img.shape
        if self.results.multi_hand_landmarks:
            for handType, handLms in zip(self.results.multi_handedness, self.results.multi_hand_landmarks):
                myHand = {}
                ## lmList
                mylmList = []
                xList = []
                yList = []
                for id, lm in enumerate(handLms.landmark):
                    px, py, pz = int(lm.x * w), int(lm.y * h), int(lm.z * w)
                    mylmList.append([px, py, pz])
                    xList.append(px)
                    yList.append(py)

                ## bbox
                xmin, xmax = min(xList), max(xList)
                ymin, ymax = min(yList), max(yList)
                boxW, boxH = xmax - xmin, ymax - ymin
                bbox = xmin, ymin, boxW, boxH
                cx, cy = bbox[0] + (bbox[2] // 2), \
                         bbox[1] + (bbox[3] // 2)

                myHand["lmList"] = mylmList
                myHand["bbox"] = bbox
                myHand["center"] = (cx, cy)

                if flipType:
                    if handType.classification[0].label == "Right":
                        myHand["type"] = "Left"
                    else:
                        myHand["type"] = "Right"
                else:
                    myHand["type"] = handType.classification[0].label
                allHands.append(myHand)

                ## draw
                if draw_connections:
                    self.mpDraw.draw_landmarks(img, handLms,
                                               self.mpHands.HAND_CONNECTIONS)
                if draw_box:
                    cv2.rectangle(img, (bbox[0] - 20, bbox[1] - 20),
                                  (bbox[0] + bbox[2] + 20, bbox[1] + bbox[3] + 20),
                                  (255, 0, 255), 2)
                    cv2.putText(img, myHand["type"], (bbox[0] - 30, bbox[1] - 30), cv2.FONT_HERSHEY_PLAIN,
                                2, (255, 0, 255), 2)
        if draw_box or draw_connections:
            return allHands, img
        else:
            return allHands



# import cv2
# import mediapipe as mp
# mp_drawing = mp.solutions.drawing_utils
# mp_drawing_styles = mp.solutions.drawing_styles
# mp_pose = mp.solutions.pose


# # For webcam input:
# cap = cv2.VideoCapture(0)
# with mp_pose.Pose(
#     min_detection_confidence=0.5,
#     min_tracking_confidence=0.5) as pose:
#   while cap.isOpened():
#     success, image = cap.read()
#     if not success:
#       print("Ignoring empty camera frame.")
#       # If loading a video, use 'break' instead of 'continue'.
#       continue

#     # To improve performance, optionally mark the image as not writeable to
#     # pass by reference.
#     image.flags.writeable = False
#     image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
#     results = pose.process(image)

#     # Draw the pose annotation on the image.
#     image.flags.writeable = True
#     image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
#     mp_drawing.draw_landmarks(
#         image,
#         results.pose_landmarks,
#         mp_pose.POSE_CONNECTIONS,
#         landmark_drawing_spec=mp_drawing_styles.get_default_pose_landmarks_style())
#     # Flip the image horizontally for a selfie-view display.
#     cv2.imshow('MediaPipe Pose', cv2.flip(image, 1))
#     if cv2.waitKey(5) & 0xFF == 27:
#       break
# cap.release()



"""
***   UTILITY FUNCTIONS   ***
"""
def aspectRationize(img, targetSize, colorFiller = None):
    """
    Resize image keeping aspect ratio and filling the gaps
    If img tall then set that to targetSize and colorfill the sides to make square
    If img fat then set that to targetSize and colorfill the up and down to make square
    if no color filler then return img without filler 
    """
    h,w,c = img.shape
    ratio = h / w
    bg_filler = None
    if colorFiller is not None:
        bg_filler = np.full((targetSize, targetSize,3), colorFiller, np.uint8) 

    if ratio > 1: # if tall
        k = targetSize / h
        wCal = math.ceil(k * w)
        img = cv2.resize(img, (wCal, targetSize))
        if colorFiller:
            wGap = math.ceil((targetSize - wCal) / 2)
            bg_filler[:, wGap:wCal + wGap] = img

    else: # if fat
        k = targetSize / w
        hCal = min(math.ceil(k * h),targetSize)
        img = cv2.resize(img, (targetSize, hCal))
        if colorFiller:
            hGap = math.ceil((targetSize - hCal) / 2)
            bg_filler[hGap:hCal + hGap, :] = img
    
    return bg_filler if colorFiller else img
