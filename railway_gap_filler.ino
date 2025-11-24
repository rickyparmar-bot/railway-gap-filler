/*
================================================================================
  RAILWAY PLATFORM GAP FILLER - MAIN CONTROL SYSTEM
================================================================================
  
  Project: Automated Railway Platform Gap Filler
  Author: Ricky Parmar
  GitHub: https://github.com/rickyparmar-bot/railway-gap-filler
  Website: https://railway-guardian-7b102bb6.base44.app/
  License: MIT License
  Date: November 2024
  
  Description:
  This Arduino program controls an automated platform gap filler system for
  railway stations. It uses an HC-SR04 ultrasonic sensor to detect approaching
  trains and automatically deploys a servo-driven scissor lift mechanism to
  bridge the gap between the platform and train, ensuring safe boarding.
  
  Hardware Components Required:
  - Arduino Uno or Nano (ATmega328P microcontroller)
  - HC-SR04 Ultrasonic Distance Sensor
  - Servo Motor (e.g., MG995 or SG90)
  - Scissor Lift Mechanism (3D printed or mechanical)
  - Power Supply (5V for Arduino, 5V-6V for servo)
  - Jumper wires and breadboard
  
  Pin Connections:
  - HC-SR04 Trig Pin  -> Arduino Pin 9
  - HC-SR04 Echo Pin  -> Arduino Pin 10
  - HC-SR04 VCC       -> Arduino 5V
  - HC-SR04 GND       -> Arduino GND
  - Servo Signal Wire -> Arduino Pin 11
  - Servo VCC (Red)   -> External 5V (or Arduino 5V for small servos)
  - Servo GND (Brown) -> Arduino GND
  
  How It Works:
  1. Ultrasonic sensor continuously measures distance to detect trains
  2. When a train is detected within 25cm, the platform deploys
  3. Servo motor rotates to 90 degrees, extending the scissor lift
  4. When train moves away (distance > 25cm), platform retracts
  5. Servo returns to 0 degrees, retracting the scissor lift
  6. System prints status messages to Serial Monitor for debugging
  
  Safety Features:
  - Only deploys when object detected within safe range (0-25cm)
  - Automatically retracts when object moves away
  - Simple and reliable sensor-based control
  
================================================================================
*/

// Include the Servo library to control servo motors
// This library provides functions like attach(), write(), and read()
#include <Servo.h>

/*
================================================================================
  PIN DEFINITIONS
================================================================================
  These constants define which Arduino pins are connected to which components.
  Using const int ensures these values can't be accidentally changed during runtime.
*/

// HC-SR04 Ultrasonic Sensor Pins
const int trigPin = 9;    // Trigger pin: sends out ultrasonic pulse
                          // Connected to TRIG pin on HC-SR04 sensor
                          // This pin will be set HIGH for 10 microseconds to emit sound

const int echoPin = 10;   // Echo pin: receives reflected ultrasonic pulse
                          // Connected to ECHO pin on HC-SR04 sensor  
                          // Stays HIGH for duration proportional to distance

// Servo Motor Pin
const int servoPin = 11;  // PWM pin for servo motor control signal
                          // Connected to signal wire (usually orange/yellow) of servo
                          // Must be a PWM-capable pin (3,5,6,9,10,11 on Uno)

/*
================================================================================
  GLOBAL OBJECTS AND VARIABLES
================================================================================
*/

// Create a Servo object to control the scissor lift mechanism
Servo platformServo;      // This object represents our servo motor
                          // We'll use it to rotate the servo to extend/retract the platform

// Variables to store distance measurements
long duration;            // Stores the time (in microseconds) the echo pin stays HIGH
                          // This is the round-trip time for the ultrasonic pulse
                          // Type 'long' can store large numbers (up to 2,147,483,647)

long distance;            // Stores the calculated distance to the object in centimeters
                          // Calculated using: distance = (duration * speed of sound) / 2
                          // Division by 2 because sound travels to object and back

/*
================================================================================
  SETUP FUNCTION - Runs once when Arduino starts or resets
================================================================================
  This function initializes all hardware components and prepares the system
  for operation. It runs only ONCE at power-up or after pressing reset button.
*/
void setup() {
  
  // Configure ultrasonic sensor pins
  // The trigPin sends out the ultrasonic pulse, so it's an OUTPUT
  pinMode(trigPin, OUTPUT);  // Set trigPin as OUTPUT to send trigger pulse
                             // This allows us to control when sensor emits sound
  
  // The echoPin receives the reflected pulse, so it's an INPUT  
  pinMode(echoPin, INPUT);   // Set echoPin as INPUT to receive echo pulse
                             // This allows us to read the timing of returned pulse
  
  // Attach the servo motor to its control pin
  platformServo.attach(servoPin);  // Initialize servo on pin 11
                                   // This enables PWM signal generation for servo control
                                   // Servo library handles the 50Hz PWM frequency automatically
  
  // Set servo to initial position (platform retracted)
  platformServo.write(0);   // Rotate servo to 0 degrees (fully retracted position)
                            // This is the "home" position where platform is flush with station
                            // write() sends PWM signal to position servo at specified angle
                            // 0 degrees = minimum position (typically 1ms pulse width)
  
  // Initialize Serial communication for debugging
  Serial.begin(9600);       // Start Serial Monitor at 9600 bits per second (baud rate)
                            // This allows us to see distance readings and system status
                            // Open Tools > Serial Monitor in Arduino IDE to view messages
                            // 9600 baud is standard and reliable for debugging
  
  // Print startup message
  Serial.println("Railway Platform Gap Filler - System Started");
  Serial.println("Monitoring for trains...");
  Serial.println("Distance threshold: 0-25cm");
  Serial.println("");
}

/*
================================================================================
  MAIN LOOP FUNCTION - Runs continuously after setup() completes
================================================================================
  This is the heart of the program. It runs forever in an infinite loop,
  constantly measuring distance and controlling the platform accordingly.
  The loop executes thousands of times per second.
*/
void loop() {
  
  /*
  ---------------------------------------------------------------------------
  STEP 1: TRIGGER THE ULTRASONIC SENSOR
  ---------------------------------------------------------------------------
  The HC-SR04 sensor works by sending out an ultrasonic pulse and measuring
  how long it takes for the echo to return. We need to trigger this pulse.
  */
  
  // First, ensure trigger pin is LOW (clean state)
  digitalWrite(trigPin, LOW);      // Set trigger pin to LOW voltage (0V)
                                   // This ensures we start from a clean state
  delayMicroseconds(2);            // Wait 2 microseconds to stabilize
                                   // This brief pause ensures clean signal
  
  // Send a 10-microsecond HIGH pulse to trigger the sensor
  digitalWrite(trigPin, HIGH);     // Set trigger pin HIGH (5V)
                                   // This tells HC-SR04 to emit ultrasonic burst
  delayMicroseconds(10);           // Keep it HIGH for 10 microseconds
                                   // HC-SR04 requires minimum 10μs trigger pulse
                                   // This causes sensor to emit 8 cycles of 40kHz ultrasound
  digitalWrite(trigPin, LOW);      // Set trigger pin back to LOW
                                   // Pulse is complete, sensor now emits ultrasonic waves
  
  /*
  ---------------------------------------------------------------------------
  STEP 2: READ THE ECHO PULSE DURATION
  ---------------------------------------------------------------------------
  After triggering, the HC-SR04 emits ultrasonic waves. When waves hit an
  object, they bounce back. The echo pin goes HIGH when echo is received.
  We measure how long the echo pin stays HIGH to calculate distance.
  */
  
  // Read the echo pin and measure pulse duration
  duration = pulseIn(echoPin, HIGH);  // Measure time (in microseconds) echo pin is HIGH
                                      // pulseIn() waits for pin to go HIGH, then times how long it stays HIGH
                                      // Returns the duration in microseconds
                                      // If no echo received within 1 second, returns 0
                                      // The longer the duration, the farther away the object
  
  /*
  ---------------------------------------------------------------------------
  STEP 3: CALCULATE DISTANCE FROM DURATION
  ---------------------------------------------------------------------------
  Distance calculation formula: distance = (duration × speed of sound) / 2
  
  Why this formula?
  - Sound travels at approximately 343 meters/second (at 20°C)
  - This equals 0.034 cm/microsecond
  - Duration is round-trip time (to object and back)
  - So we divide by 2 to get one-way distance
  
  Formula breakdown:
  distance = (duration in μs) × (0.034 cm/μs) / 2
  distance = duration × 0.017 cm/μs
  OR equivalently:
  distance = (duration / 2) × 0.034
  distance = duration × 0.034 / 2  <-- This is what we use
  */
  
  distance = duration * 0.034 / 2;   // Convert duration to distance in centimeters
                                     // Multiply by 0.034 (speed of sound in cm/μs)
                                     // Divide by 2 (sound travels to object and back)
                                     // Result is distance in centimeters
  
  /*
  ---------------------------------------------------------------------------
  STEP 4: DECISION LOGIC - DEPLOY OR RETRACT PLATFORM
  ---------------------------------------------------------------------------
  Based on the measured distance, decide whether to deploy or retract platform:
  - If train detected (distance between 0 and 25cm): Deploy platform
  - If no train detected (distance > 25cm): Retract platform
  
  The 25cm threshold was chosen because:
  - Close enough to reliably detect train
  - Far enough to avoid false triggers from people
  - Gives platform time to deploy before train fully arrives
  */
  
  // Check if object (train) detected within deployment range
  if (distance > 0 && distance < 25) {
    // CONDITION EXPLAINED:
    // distance > 0: Ensures sensor got valid reading (0 means no echo/error)
    // distance < 25: Object is within 25cm (detection threshold)
    // Both must be true (AND logic) for deployment
    
    /*
    *** DEPLOY PLATFORM ***
    Train detected nearby - extend scissor lift to bridge gap
    */
    
    platformServo.write(90);        // Rotate servo to 90 degrees (extended position)
                                    // This extends the scissor lift mechanism
                                    // 90° positions servo at midpoint (typically 1.5ms pulse)
                                    // Adjust this value based on your scissor lift design:
                                    // - Increase (e.g., 120°) for more extension
                                    // - Decrease (e.g., 60°) for less extension
    
    // Print status to Serial Monitor for debugging
    Serial.println("Platform deployed");  // Confirmation message
                                           // Shows in Serial Monitor that deployment occurred
    
  } else {
    // ELSE CONDITION:
    // Either no object detected (distance = 0) OR object too far (distance >= 25cm)
    // In both cases, we want platform retracted for safety
    
    /*
    *** RETRACT PLATFORM ***
    No train detected or train moved away - retract scissor lift
    */
    
    platformServo.write(0);         // Rotate servo to 0 degrees (retracted position)
                                    // This retracts the scissor lift flush with platform
                                    // 0° is minimum position (typically 1ms pulse width)
                                    // Platform is now safe and out of the way
  }
  
  /*
  ---------------------------------------------------------------------------
  STEP 5: LOOP DELAY
  ---------------------------------------------------------------------------
  Add small delay before next measurement cycle to prevent sensor overload
  and reduce unnecessary servo jitter.
  */
  
  delay(100);                      // Wait 100 milliseconds (0.1 seconds) before next loop
                                   // This creates a measurement frequency of ~10 times/second
                                   // Benefits:
                                   // - Prevents sensor from being triggered too rapidly
                                   // - Reduces servo jitter from constant updates  
                                   // - Makes Serial Monitor output readable
                                   // - Still fast enough for real-time train detection
                                   // You can adjust this value:
                                   // - Decrease (e.g., 50) for faster response
                                   // - Increase (e.g., 200) for slower, smoother operation
}

/*
================================================================================
  END OF PROGRAM
================================================================================

  USAGE INSTRUCTIONS:
  1. Connect hardware according to pin definitions above
  2. Upload this code to your Arduino board
  3. Open Serial Monitor (Tools > Serial Monitor) at 9600 baud
  4. Test by placing object within 25cm of sensor
  5. Observe servo movement and Serial Monitor messages
  
  TROUBLESHOOTING:
  - Platform doesn't deploy: Check sensor connections and 25cm threshold
  - Servo jitters: Use external power supply for servo, add capacitor
  - Distance always 0: Check echo pin connection, sensor power
  - Platform deploys randomly: Increase distance threshold or add debouncing
  
  CALIBRATION TIPS:
  - Adjust platformServo.write(90) value to control extension amount
  - Adjust platformServo.write(0) value if retracted position isn't flush
  - Change "distance < 25" threshold based on your station layout
  - Add delay after servo movement for mechanical settling
  
  SAFETY NOTES:
  - Test thoroughly with model trains before real deployment
  - Ensure scissor lift mechanism is sturdy and reliable
  - Add emergency stop button for manual control
  - Consider adding multiple sensors for redundancy
  - Servo should be powerful enough for your scissor lift weight
  
  FUTURE ENHANCEMENTS:
  - Add LED indicators for system status
  - Add buzzer for audio alerts
  - Implement smooth servo movement (gradual rotation)
  - Add debouncing to prevent false triggers
  - Use multiple sensors for better accuracy
  - Add emergency stop button
  - Log deployment events to SD card
  - Add wireless monitoring via ESP8266/ESP32
  
================================================================================
*/
