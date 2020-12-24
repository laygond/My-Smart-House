
import cv2
import numpy as np


# TODO create config file and add to gitignore 

cam_addresses = [
# 'protocol://username:password@IP:port/1' 
# IP Webcam Android App works for webcam
#"https://192.168.1.128:8183/video"
#"rtsp://192.168.1.128:8183/h264_ulaw.sdp"
#"http://192.168.1.128:8183/videofeed"
#"rtsp://192.168.1.128:8183/h264_pcm.sdp"
#"https://192.168.1.128:8183/onvif/device_service"

# # Wyze version of RTSP
# "rtsp://10.0.0.7/live",
#"rtsp://admin:____@192.168.1.12/live",
"rtsp://admin:ZECP1@192.168.68.64/live",

# # DroidCam
# "http://10.0.0.4:4747/video",

# # MotionEyeOS
"http://10.0.0.8:8081",

# # EZVIZ
# rtsp://admin:verification_code(located on camera sticker)@IP:554/H.264
#"rtsp://admin:_____@192.168.1.13:554/H.264",


# Webcam
0
]


# No Signal and No Camera Case
no_signal = cv2.imread("no_signal.png")
no_signal = cv2.resize(no_signal,(320,240))
no_camera = np.zeros_like(no_signal)
no_signal = cv2.copyMakeBorder(no_signal, 10, 10, 10, 10, cv2.BORDER_CONSTANT, value=(255,255,255)) 
no_camera = cv2.copyMakeBorder(no_camera, 10, 10, 10, 10, cv2.BORDER_CONSTANT, value=(255,255,255))

# Load Video Cameras
cameras = []
for address in cam_addresses:
    try:
        cam = cv2.VideoCapture(address)
        cameras.append(cam)
    except:
        print("[WARNING] Failed to conecct to ", address)
        pass

while(True):
    frames = []
    # Read Frame for each camera
    for cam in cameras:
        ret, frame = cam.read()
        if ret:
            frame = cv2.resize(frame,(320,240))
            frame = cv2.copyMakeBorder(frame, 10, 10, 10, 10, cv2.BORDER_CONSTANT, value=(255,255,255))
        else:
            frame = no_signal
        frames.append(frame)

    # Display
    if len(frames) == 1:
        monitor = frames[0]
    
    elif len(frames) == 2:
        monitor = np.hstack((frames[0],frames[1]))
    
    elif len(frames) > 2:
        if len(frames) % 2 == 1:
            frames.append(no_camera)
        half_n_frames = int(len(frames)/2)
        monitor_row_1 = frames[0]
        monitor_row_2 = frames[half_n_frames]
        for i in range(1,half_n_frames):
            monitor_row_1 = np.hstack((monitor_row_1,frames[i]))
        for i in range(half_n_frames+1,len(frames)):
            monitor_row_2 = np.hstack((monitor_row_2,frames[i]))
        monitor = np.vstack((monitor_row_1, monitor_row_2))
    
    cv2.imshow("Monitor", monitor)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

for cam in cameras:
    cam.release()
cv2.destroyAllWindows()

# # import requests
# # import numpy as np
# # import cv2
# # while True:
# #     img_res = requests.get("http://192.168.0.103:8080/shot.jpg")
# #     img_arr = np.array(bytearray(img_res.content), dtype = np.uint8)
# #     img = cv2.imdecode(img_arr,-1)

# #     cv2.imshow('frame', img)
# #     if cv2.waitKey(1) & 0xFF == ord('q'):
# #         break