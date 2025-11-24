/*
 * Railway Gap Filler - Automated Platform Safety System
 * 
 * Description: Arduino-powered automated platform gap filler for railway stations.
 *              Smart safety solution using ultrasonic sensors and servo-driven
 *              scissor lift mechanism for accessible transportation.
 * 
 * Author: Ricky Parmar
 * GitHub: https://github.com/rickyparmar-bot/railway-gap-filler
 * Website: https://railway-guardian-7b102bb6.base44.app/
 * 
 * Hardware:
 * - Arduino Uno/Nano (ATmega328P)
 * - HC-SR04 Ultrasonic Sensor
 * - MG995 Servo Motors (2+)
 * - Scissor Lift Mechanism
 * - Power Supply (5V/12V)
 * 
 * License: MIT License
 * Date: November 2024
 */

#include <Servo.h>

// ===== PIN DEFINITIONS =====
// HC-SR04 Ultrasonic Sensor Pins
const int TRIG_PIN = 9;      // Trigger pin for ultrasonic sensor
const int ECHO_PIN = 10;     // Echo pin for ultrasonic sensor

// Servo Motor Pins
const int SERVO1_PIN = 5;    // First servo motor
const int SERVO2_PIN = 6;    // Second servo motor

// Optional: LED and Buzzer Pins
const int LED_GREEN = 2;     // Green LED - System ready
const int LED_RED = 3;       // Red LED - Platform deployed
const int BUZZER_PIN = 4;    // Buzzer for audio alerts

// ===== CONFIGURATION PARAMETERS =====
const int THRESHOLD_DISTANCE = 20;    // Distance threshold in cm (train detection)
const int MIN_DISTANCE = 13;          // Minimum reliable detection distance
const int MAX_DISTANCE = 25;          // Maximum detection range

// Servo positions (adjust based on your mechanism)
const int SERVO_RETRACTED = 0;        // Servo angle when platform is retracted
const int SERVO_EXTENDED = 90;        // Servo angle when platform is extended

// Timing parameters
const int DEPLOYMENT_DELAY = 500;     // Delay for smooth deployment (ms)
const int MEASUREMENT_DELAY = 100;    // Delay between distance measurements (ms)
const int DEBOUNCE_COUNT = 3;         // Number of consistent readings required

// ===== GLOBAL VARIABLES =====
Servo servo1;                         // First servo object
Servo servo2;                         // Second servo object

bool platformDeployed = false;        // Current platform state
int consecutiveDetections = 0;        // Counter for debouncing
int consecutiveNoDetections = 0;      // Counter for train departure

float distance = 0;                   // Current measured distance

// ===== FUNCTION DECLARATIONS =====
void setup();
void loop();
float measureDistance();
void deployPlatform();
void retractPlatform();
void playBeep(int duration);
void updateLEDs();

// ===== SETUP FUNCTION =====
void setup() {
  // Initialize Serial communication for debugging
  Serial.begin(9600);
  Serial.println("Railway Gap Filler System - Initializing...");
  Serial.println("=========================================");
  
  // Configure ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Configure LED and buzzer pins
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize servos
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  
  // Set initial position (retracted)
  servo1.write(SERVO_RETRACTED);
  servo2.write(SERVO_RETRACTED);
  
  // Initial LED state
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  
  // System ready beep
  playBeep(100);
  delay(100);
  playBeep(100);
  
  Serial.println("System initialized successfully!");
  Serial.println("Monitoring for trains...");
  Serial.println("=========================================");
  Serial.println();
  
  delay(1000);
}

// ===== MAIN LOOP =====
void loop() {
  // Measure distance from sensor
  distance = measureDistance();
  
  // Print distance for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm");
  
  // Check if train is detected within threshold
  if (distance >= MIN_DISTANCE && distance <= THRESHOLD_DISTANCE) {
    consecutiveDetections++;
    consecutiveNoDetections = 0;
    
    Serial.print(" - Train detected! (");
    Serial.print(consecutiveDetections);
    Serial.print("/");
    Serial.print(DEBOUNCE_COUNT);
    Serial.println(")");
    
    // Deploy platform if consistent detection and not already deployed
    if (consecutiveDetections >= DEBOUNCE_COUNT && !platformDeployed) {
      Serial.println();
      Serial.println(">>> DEPLOYING PLATFORM <<<");
      deployPlatform();
      consecutiveDetections = 0;
    }
  }
  // Check if train has departed
  else if (distance > MAX_DISTANCE) {
    consecutiveNoDetections++;
    consecutiveDetections = 0;
    
    if (platformDeployed) {
      Serial.print(" - Train departing... (");
      Serial.print(consecutiveNoDetections);
      Serial.print("/");
      Serial.print(DEBOUNCE_COUNT);
      Serial.println(")");
    } else {
      Serial.println(" - No train detected");
    }
    
    // Retract platform if train has left and platform is deployed
    if (consecutiveNoDetections >= DEBOUNCE_COUNT && platformDeployed) {
      Serial.println();
      Serial.println(">>> RETRACTING PLATFORM <<<");
      retractPlatform();
      consecutiveNoDetections = 0;
    }
  }
  // Distance is between threshold and max (train nearby but not close enough)
  else {
    consecutiveDetections = 0;
    consecutiveNoDetections = 0;
    Serial.println(" - Train approaching...");
  }
  
  // Small delay before next measurement
  delay(MEASUREMENT_DELAY);
}

// ===== DISTANCE MEASUREMENT FUNCTION =====
float measureDistance() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10 microsecond pulse to trigger
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Timeout after 30ms
  
  // Calculate distance in cm
  // Speed of sound is 343 m/s or 0.0343 cm/microsecond
  // Distance = (Time * Speed) / 2 (divide by 2 for round trip)
  float dist = (duration * 0.0343) / 2;
  
  // Return 0 if no echo received (out of range)
  if (duration == 0) {
    return 999; // Return large value to indicate no detection
  }
  
  return dist;
}

// ===== PLATFORM DEPLOYMENT FUNCTION =====
void deployPlatform() {
  Serial.println("Deploying scissor lift mechanism...");
  
  // Play alert beep
  playBeep(200);
  
  // Update LEDs
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  
  // Smoothly move servos to extended position
  for (int angle = SERVO_RETRACTED; angle <= SERVO_EXTENDED; angle++) {
    servo1.write(angle);
    servo2.write(angle);
    delay(15); // Smooth movement
  }
  
  platformDeployed = true;
  
  Serial.println("Platform deployed successfully!");
  Serial.println("Status: ACTIVE - Bridging gap");
  Serial.println();
  
  // Success beep
  playBeep(100);
  delay(50);
  playBeep(100);
}

// ===== PLATFORM RETRACTION FUNCTION =====
void retractPlatform() {
  Serial.println("Retracting scissor lift mechanism...");
  
  // Play alert beep
  playBeep(150);
  
  // Smoothly move servos to retracted position
  for (int angle = SERVO_EXTENDED; angle >= SERVO_RETRACTED; angle--) {
    servo1.write(angle);
    servo2.write(angle);
    delay(15); // Smooth movement
  }
  
  platformDeployed = false;
  
  // Update LEDs
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  
  Serial.println("Platform retracted successfully!");
  Serial.println("Status: READY - Monitoring for trains...");
  Serial.println();
  
  // Success beep
  playBeep(100);
}

// ===== BUZZER BEEP FUNCTION =====
void playBeep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

// ===== LED UPDATE FUNCTION =====
void updateLEDs() {
  if (platformDeployed) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  } else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  }
}

/*
 * ===== USAGE INSTRUCTIONS =====
 * 
 * 1. Hardware Assembly:
 *    - Connect HC-SR04 sensor: VCC->5V, Trig->Pin 9, Echo->Pin 10, GND->GND
 *    - Connect Servo 1: Signal->Pin 5, VCC->5V, GND->GND
 *    - Connect Servo 2: Signal->Pin 6, VCC->5V, GND->GND
 *    - Optional: LEDs on pins 2 & 3, Buzzer on pin 4
 * 
 * 2. Calibration:
 *    - Adjust THRESHOLD_DISTANCE for your setup (default: 20cm)
 *    - Adjust SERVO_EXTENDED and SERVO_RETRACTED angles
 *    - Test with train model at various distances
 * 
 * 3. Operation:
 *    - Upload code to Arduino
 *    - Open Serial Monitor (9600 baud) for debugging
 *    - System will automatically detect and respond to trains
 * 
 * 4. Serial Monitor Commands:
 *    - Watch real-time distance measurements
 *    - Monitor deployment/retraction events
 *    - Debug system behavior
 * 
 * ===== TROUBLESHOOTING =====
 * 
 * Problem: Platform doesn't deploy
 * - Check THRESHOLD_DISTANCE value
 * - Verify sensor connections
 * - Check servo power supply
 * 
 * Problem: Servo jitters or doesn't move smoothly
 * - Use external power supply for servos
 * - Check servo angle values
 * - Reduce movement speed in deploy/retract functions
 * 
 * Problem: False detections
 * - Increase DEBOUNCE_COUNT
 * - Adjust MIN_DISTANCE and MAX_DISTANCE
 * - Check sensor mounting and alignment
 * 
 * ===== FUTURE ENHANCEMENTS =====
 * - Add multiple sensor support for better accuracy
 * - Implement emergency stop button
 * - Add Wi-Fi connectivity for remote monitoring
 * - Create mobile app for real-time status
 * - Add data logging for analytics
 */
