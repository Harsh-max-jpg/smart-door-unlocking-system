/*
 * Smart Door Unlocking System - ESP32 Firmware
 * ---------------------------------------------
 * Drives the door hardware (servo, relay, LEDs, LCD) on an "OPEN" or
 * "DENIED" command, and independently reports IR sensor motion status
 * on the LCD.
 *
 * COMMUNICATION: this firmware accepts commands over TWO channels,
 * both wired to the exact same doOpen()/doDeny() logic:
 *
 *   1. Wi-Fi / HTTP  - runs as an access point named by AP_SSID below,
 *      always at IP 192.168.4.1 (ESP32 default softAP address).
 *      GET http://192.168.4.1/unlock  -> doOpen()
 *      GET http://192.168.4.1/deny    -> doDeny()
 *      This matches what python/face.py already calls.
 *
 *   2. USB Serial - for manual testing via the Arduino Serial Monitor
 *      (115200 baud), send the literal text "OPEN" or "DENIED".
 *
 * IMPORTANT: for the HTTP path to work, the computer running
 * python/face.py must be connected to this ESP32's Wi-Fi network
 * (AP_SSID / AP_PASSWORD below) before running the script - see
 * docs/setup.md.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ================= WI-FI AP CONFIG =================
// The ESP32 hosts its own Wi-Fi network so python/face.py can reach it
// at the fixed address http://192.168.4.1 without needing a router.
// CHANGE THIS PASSWORD before using outside a private test setup.
const char* AP_SSID     = "SmartDoorESP32";
const char* AP_PASSWORD = "changeme123";   // must be 8+ characters for WPA2

WebServer server(80);

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

// ================= SHARED ACTIONS =================
// Both the HTTP handlers and the Serial command parser call these, so
// the two entry points can never drift into different behavior.

void doOpen() {
  // Authorized face recognized - unlock the door
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door Unlocked");

  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RELAY_PIN, LOW);   // Energize relay (unlocked)
  doorServo.write(90);           // Rotate to unlocked position

  delay(5000);                   // Hold unlocked for 5 seconds
                                  // (blocking delay: IR/Serial/HTTP are not
                                  // serviced again until this returns)

  doorServo.write(0);            // Return to locked position
  digitalWrite(RELAY_PIN, HIGH); // De-energize relay (locked)
  digitalWrite(GREEN_LED, LOW);

  lcd.clear();
  lcd.print("System Ready");
}

void doDeny() {
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

// ================= HTTP HANDLERS =================
void handleUnlock() {
  doOpen();
  server.send(200, "text/plain", "OPEN OK");
}

void handleDeny() {
  doDeny();
  server.send(200, "text/plain", "DENIED OK");
}

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

  // ================= WI-FI AP + HTTP SERVER SETUP =================
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress apIP = WiFi.softAPIP();   // Expected: 192.168.4.1
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  server.on("/unlock", handleUnlock);
  server.on("/deny", handleDeny);
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("READY");
}

void loop() {
  // Service any pending HTTP requests
  server.handleClient();

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
  // Kept for manual testing via the Serial Monitor; calls the exact
  // same functions as the HTTP routes above.
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "OPEN") {
      doOpen();
    }
    else if (cmd == "DENIED") {
      doDeny();
    }
  }
}
