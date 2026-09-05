/*
 * Smart Door Unlocking System - ESP32 Firmware
 * ---------------------------------------------
 * Receives text commands ("OPEN" / "DENIED") over the USB Serial link
 * from the PC-side face recognition script and drives the door
 * hardware (servo, relay, LEDs, LCD) accordingly. Also independently
 * reports IR sensor motion status on the LCD.
 *
 * NOTE ON COMMUNICATION PROTOCOL:
 * This firmware listens on Serial only. It does NOT run a Wi-Fi
 * access point or HTTP server. The current python/face.py script,
 * however, sends its "unlock" signal as an HTTP GET request to
 * http://192.168.4.1/unlock, not over Serial. As shipped, the two
 * halves of the project do not talk to each other automatically -
 * see the "Communication Protocol" section of the README for details
 * and options to reconcile this before relying on the system.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ================= PIN CONFIG =================
#define IR_SENSOR   13   // IR sensor OUT pin - LOW when an object/person is detected
#define RED_LED     25   // Lights up on "Access Denied"
#define GREEN_LED   26   // Lights up while the door is unlocked
#define RELAY_PIN   27   // Drives the door lock relay (active-LOW module: LOW = energized)
#define SERVO_PIN   14   // Signal pin for the door servo

Servo doorServo;

// LCD I2C address. This module was confirmed to work at 0x27.
// If your LCD backpack uses a different address (0x3F is common),
// change it here - an I2C scanner sketch can help identify it.
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool motionDetected = false;

void setup() {
  Serial.begin(115200);

  pinMode(IR_SENSOR, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Relay module used here is active-LOW: HIGH = door locked (idle/safe state)
  digitalWrite(RELAY_PIN, HIGH);

  // ================= SERVO SETUP =================
  doorServo.setPeriodHertz(50);   // Standard 50Hz servo PWM frequency
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);             // 0 degrees = locked position on startup

  // ================= LCD SETUP =================
  Wire.begin(21, 22);   // ESP32 default I2C pins: SDA = GPIO21, SCL = GPIO22
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");

  Serial.println("READY");
}

void loop() {
  // ---------------- IR SENSOR STATUS ----------------
  // Purely local status display; does not gate the OPEN/DENIED logic below.
  int motion = digitalRead(IR_SENSOR);

  if (motion == LOW && !motionDetected) {
    motionDetected = true;
    Serial.println("TRIGGER");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Object Detected");
  }

  if (motion == HIGH && motionDetected) {
    motionDetected = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Object");
  }

  // ---------------- SERIAL COMMAND HANDLING ----------------
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "OPEN") {
      // Authorized face recognized - unlock the door
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Door Unlocked");

      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RELAY_PIN, LOW);   // Energize relay (unlocked)
      doorServo.write(90);           // Rotate to unlocked position

      delay(5000);                   // Hold unlocked for 5 seconds
                                      // (blocking delay: IR/Serial are not
                                      // serviced again until this returns)

      doorServo.write(0);            // Return to locked position
      digitalWrite(RELAY_PIN, HIGH); // De-energize relay (locked)
      digitalWrite(GREEN_LED, LOW);

      lcd.clear();
      lcd.print("System Ready");
    }
    else if (cmd == "DENIED") {
      // Unauthorized face - flash red LED and deny access
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access Denied");

      digitalWrite(RED_LED, HIGH);
      delay(2000);
      digitalWrite(RED_LED, LOW);

      lcd.clear();
      lcd.print("Try Again");
      delay(1500);

      lcd.clear();
      lcd.print("System Ready");
    }
  }
}
