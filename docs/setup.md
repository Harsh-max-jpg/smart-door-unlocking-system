# Setup Guide

## 1. Clone the repository

```bash
git clone <YOUR_REPO_URL>
cd Smart-Door-Unlocking-System
```

## 2. Python environment

```bash
cd python
python3 -m venv venv        # optional but recommended
source venv/bin/activate    # Windows: venv\Scripts\activate
pip install -r ../requirements.txt
```

## 3. Arduino IDE / ESP32 setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add ESP32 board support: **File → Preferences → Additional Boards
   Manager URLs**, add
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`,
   then install "esp32" via **Tools → Board → Boards Manager**.
3. Install the required libraries via **Sketch → Include Library →
   Manage Libraries**:
   - `LiquidCrystal_I2C`
   - `ESP32Servo`
   - `Wire` is bundled with the ESP32 core (no separate install needed).
4. Open `arduino/smart_door_esp32/smart_door_esp32.ino`.
5. Select **Tools → Board → ESP32 Dev Module** (or your specific
   board) and the correct **Port**.
6. Click **Upload**.

## 4. Running the Python side

All commands below are run from inside the `python/` directory, since
the scripts reference `dataset/`, `trainer.yml`, and the haarcascade
file using relative paths.

```bash
cd python

# (Optional) verify your webcam works
python test_cam.py

# Step 1: collect face images for a user
# You will be prompted for a numeric user ID; captures 30 face crops
python dataset.py

# Step 2: train the LBPH recognizer on everything in dataset/
python trainer.py

# Step 3: run real-time recognition
python face.py
```

Press **Esc** to exit `dataset.py` or `face.py`.

## 5. Before relying on ESP32 unlock behavior

Read [`architecture.md`](architecture.md) first: as shipped,
`face.py` sends an HTTP request that the current firmware cannot
receive (the firmware only listens on Serial). You will need to make
one of the two changes described there before an authorized face
actually triggers the servo/relay on the ESP32.

## 6. Repeating for multiple users

Run `python dataset.py` again with a different numeric ID for each
additional person, then re-run `python trainer.py` to retrain on the
combined dataset before running `face.py` again.
