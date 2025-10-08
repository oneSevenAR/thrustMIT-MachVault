// A4988 Dual Stepper — independent non-blocking stepping
// Stepper 1: STEP D3, DIR D4, MS1 D8, MS2 D9, MS3 D10
// Stepper 2: STEP D5, DIR D6, MS1 D11, MS2 D12, MS3 D13

// follower
const int stepPin1 = 3;
const int dirPin1 = 4;
const int ms1_1 = 8;
const int ms2_1 = 9;
const int ms3_1 = 10;

// rotor
const int stepPin2 = 5;
const int dirPin2 = 6;
const int ms1_2 = 11;
const int ms2_2 = 12;
const int ms3_2 = 13;

// --- Timing parameters ---
// Stepper 2 controls speed; Stepper 1 = 0.5 × Stepper 2 speed
unsigned long period2_us = 1000;    // Stepper 2 total period (HIGH + LOW in microseconds)
unsigned long pulseHigh2_us = 500;  // Stepper 2 pulse HIGH time (microseconds)

unsigned long period1_us = period2_us * 100; // Stepper 1 total period (80 times slower than M2)
unsigned long pulseHigh1_us = 1000;     // Stepper 1 pulse HIGH time

// initial directions
bool dirState1 = true;  // HIGH (Initial direction for M1)
bool dirState2 = false; // LOW (Initial direction for M2)

// ----------------------------------------------------
// ⭐ CONFIGURATION PARAMETER: Stepper 1 Distance Control
// Adjust this value to set the distance (in steps) Stepper 1 travels *during the oscillating coiling phase*.
const long MAX_STEPS_1 =810 ; 
// ----------------------------------------------------

// ⚡ CRITICAL CONFIGURATION PARAMETER: Deadzone Control
// Set the distance (in steps) Stepper 1 travels *one-way* to clear the deadzone (1.5 cm).
// ⚠️ YOU MUST ADJUST THIS VALUE BASED ON YOUR HARDWARE (e.g., motor steps/rev, leadscrew pitch, microstepping mode).
const long DEADZONE_STEPS_1 =150 ; // Example value for 1.5 cm
// ----------------------------------------------------

// --- Internal timing/state ---
unsigned long lastEdgeTime1 = 0;
unsigned long lastEdgeTime2 = 0;

// stepState: 0 = waiting for next rising edge (LOW period), 1 = pulse is HIGH
uint8_t stepState1 = 0;
uint8_t stepState2 = 0;

// Tracks M1 steps for reversal
long stepsTaken1 = 0;

// State: 0 = Moving OUT of deadzone, 1 = Moving IN (coiling), 2 = Moving OUT (coiling)
int motionState1 = 0; 

// Generalized function to set microstepping pins for any stepper
void setMicrostepping(int mode, int ms1, int ms2, int ms3) {
switch(mode) {
 case 1:  digitalWrite(ms1, LOW); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break; // Full step
 case 2:  digitalWrite(ms1, HIGH); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break; // Half step
 case 4:  digitalWrite(ms1, LOW); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break; // Quarter step
 case 8:  digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break; // Eighth step
 case 16: digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, HIGH); break; // Sixteenth step
 default: digitalWrite(ms1, LOW); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
}
}

void setup() {
// Configure all pins as OUTPUT
pinMode(stepPin1, OUTPUT);
pinMode(dirPin1, OUTPUT);
pinMode(ms1_1, OUTPUT);
pinMode(ms2_1, OUTPUT);
pinMode(ms3_1, OUTPUT);

pinMode(stepPin2, OUTPUT);
pinMode(dirPin2, OUTPUT);
pinMode(ms1_2, OUTPUT);
pinMode(ms2_2, OUTPUT);
pinMode(ms3_2, OUTPUT);

// Start Serial for optional debugging
Serial.begin(9600);
Serial.println("Stepper initialization complete. M1 moving to clear deadzone.");

// Set initial directions
digitalWrite(dirPin1, dirState1 ? HIGH : LOW); // Start moving to clear deadzone
digitalWrite(dirPin2, dirState2 ? HIGH : LOW);

// Set microstepping modes
setMicrostepping(2, ms1_1, ms2_1, ms3_1);  // Stepper 1: Half step (2x)
setMicrostepping(4, ms1_2, ms2_2, ms3_2);  // Stepper 2: Quarter step (4x)

// Ensure step pins are LOW initially
digitalWrite(stepPin1, LOW);
digitalWrite(stepPin2, LOW);

// ⚡ FIX: Initialize timing reference to ensure immediate start 
// We set the last edge time back by one full period to force the step condition to be true 
// immediately in the first loop() call.
unsigned long now = micros();
lastEdgeTime1 = now - period1_us;
lastEdgeTime2 = now - period2_us;
}

void loop() {
unsigned long now = micros();

// ---------- Motor 1 (Oscillating Motion Controller: Deadzone & Coiling) ----------
// Non-blocking check for the start of the step pulse
if (stepState1 == 0) { 
 if (now - lastEdgeTime1 >= period1_us) {
 digitalWrite(stepPin1, HIGH);
 lastEdgeTime1 = now;
 stepState1 = 1;
 }
} 
// Non-blocking check for the end of the step pulse
else { 
 if (now - lastEdgeTime1 >= pulseHigh1_us) {
 digitalWrite(stepPin1, LOW);
 lastEdgeTime1 = now;
 stepState1 = 0;
 
 // ⭐ M1 Step Counting and Reversal Logic (Deadzone + Coiling)
 stepsTaken1++;
 
 if (motionState1 == 0) {
  // State 0: Moving OUT of deadzone (1.5cm clearance)
  if (stepsTaken1 >= DEADZONE_STEPS_1) {
   // Reached the deadzone limit. Reverse direction to move IN.
   stepsTaken1 = 0; // Reset counter for the oscillating phase
   dirState1 = !dirState1; // Reverse direction
   digitalWrite(dirPin1, dirState1 ? HIGH : LOW);
   motionState1 = 1; // Change to State 1 (Moving IN - Start Coiling)
   Serial.println("DEADZONE cleared. Stepper 1 starting IN-motion (coiling).");
  }
 } 
 else if (motionState1 == 1) {
  // State 1: Moving IN (Coiling phase)
  if (stepsTaken1 >= MAX_STEPS_1) {
   // Reached MAX_STEPS_1 for the IN motion. Reverse direction to move OUT.
   stepsTaken1 = 0; // Reset counter
   dirState1 = !dirState1; // Reverse direction
   digitalWrite(dirPin1, dirState1 ? HIGH : LOW);
   motionState1 = 2; // Change to State 2 (Moving OUT - Coiling)
   Serial.println("Stepper 1 reversed. Moving OUT (coiling).");
  }
 }
 else if (motionState1 == 2) {
  // State 2: Moving OUT (Coiling phase)
  if (stepsTaken1 >= MAX_STEPS_1) {
   // Reached MAX_STEPS_1 for the OUT motion. Reverse direction to move IN.
   stepsTaken1 = 0; // Reset counter
   dirState1 = !dirState1; // Reverse direction
   digitalWrite(dirPin1, dirState1 ? HIGH : LOW);
   motionState1 = 1; // Change back to State 1 (Moving IN - Coiling)
   Serial.println("Stepper 1 reversed. Moving IN (coiling).");
  }
 }
 }
}

// ---------- Motor 2 (Full speed - Continuous run) ----------
// Non-blocking check for the start of the step pulse
if (stepState2 == 0) {
 if (now - lastEdgeTime2 >= period2_us) {
 digitalWrite(stepPin2, HIGH);
 lastEdgeTime2 = now;
 stepState2 = 1;
 }
} 
// Non-blocking check for the end of the step pulse
else { 
 if (now - lastEdgeTime2 >= pulseHigh2_us) {
 digitalWrite(stepPin2, LOW);
 lastEdgeTime2 = now;
 stepState2 = 0;
 }
}
}