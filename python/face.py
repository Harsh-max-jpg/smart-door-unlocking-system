import cv2
import numpy as np
import requests
import time

# =========================
# CONFIGURATION
# =========================
ESP32_URL = "http://192.168.4.1/unlock"
CONFIDENCE_THRESHOLD = 60   # Slightly relaxed for better detection

# =========================
# AUTO CAMERA DETECTION
# =========================
def get_camera():
    for i in range(5):  # Try 0 to 4
        cam = cv2.VideoCapture(i)
        if cam.isOpened():
            print(f"Using camera index: {i}")
            return cam
    print("❌ No camera found!")
    exit()

# =========================
# LOAD MODELS
# =========================
recognizer = cv2.face.LBPHFaceRecognizer_create()

try:
    recognizer.read("trainer.yml")
except:
    print("❌ trainer.yml not found! Run trainer.py first")
    exit()

faceCascade = cv2.CascadeClassifier("haarcascade_frontalface_default.xml")

if faceCascade.empty():
    print("❌ Haarcascade file missing!")
    exit()

# =========================
# START CAMERA
# =========================
cam = get_camera()
cam.set(3, 640)
cam.set(4, 480)

print("✅ System Started... Looking for faces")

last_unlock_time = 0
last_status = ""

# =========================
# MAIN LOOP
# =========================
while True:
    ret, img = cam.read()

    if not ret:
        print("❌ Camera error")
        break

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    faces = faceCascade.detectMultiScale(
        gray,
        scaleFactor=1.2,
        minNeighbors=5,
        minSize=(80, 80)
    )

    if len(faces) == 0:
        last_status = "No Face Detected"

    for (x, y, w, h) in faces:

        cv2.rectangle(img, (x,y), (x+w,y+h), (0,255,0), 2)

        id, confidence = recognizer.predict(gray[y:y+h, x:x+w])
        confidence_percent = round(100 - confidence)

        if confidence < CONFIDENCE_THRESHOLD:
            name = "AUTHORIZED"
            color = (0,255,0)
            last_status = "Access Granted"

            # Avoid multiple triggers
            if time.time() - last_unlock_time > 5:
                try:
                    print("🔓 Unlocking Door...")
                    requests.get(ESP32_URL, timeout=2)
                    last_unlock_time = time.time()
                except:
                    print("⚠️ ESP32 not reachable")

        else:
            name = "UNKNOWN"
            color = (0,0,255)
            last_status = "Access Denied"

        # Display name
        cv2.putText(img, name, (x, y-10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.9, color, 2)

        # Display confidence
        cv2.putText(img, f"{confidence_percent}%",
                    (x, y+h+20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, color, 2)

    # Display system status
    cv2.putText(img, last_status, (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255,255,255), 2)

    cv2.imshow('Smart Door System', img)

    # Exit on ESC
    if cv2.waitKey(1) == 27:
        break

# =========================
# CLEANUP
# =========================
print("Exiting...")
cam.release()
cv2.destroyAllWindows()