// A4988 Dual Stepper — independent non-blocking stepping
// Motor 1 (Coiling) controls the cycle:
// COILING: M1 Forward (1275 steps for 5cm), M2 ON
// RETURNING: M1 Backward (1275 steps to return), M2 OFF

// Stepper 1 pins
const int stepPin1 = 3;
const int dirPin1 = 4;
const int ms1_1 = 8;
const int ms2_1 = 9;
const int ms3_1 = 10;

// Stepper 2 pins
const int stepPin2 = 5;
const int dirPin2 = 6;
const int ms1_2 = 11;
const int ms2_2 = 12;
const int ms3_2 = 13;

// --- Timing parameters ---
unsigned long period2_us = 1000;    // Stepper 2 total period (HIGH + LOW)
unsigned long pulseHigh2_us = 500;   // Stepper 2 pulse HIGH time

unsigned long period1_us = period2_us * 40; // Stepper 1 total period
unsigned long pulseHigh1_us = 1000;     // Stepper 1 pulse HIGH time

// initial directions
bool dirState1 = true;  // HIGH
bool dirState2 = false; // LOW

// --- Motor 1 Cycle Limit ---
// Empirically derived steps for 5 cm of material
const long MAX_STEPS_1 = 1148; 

// --- State Management ---
enum CycleState {
    COILING,  // M1 Forward (Winding), M2 On (Traversing)
    RETURNING // M1 Backward (Returning), M2 Off (Stopped)
};
CycleState currentCycleState = COILING;

// --- Internal timing/state ---
unsigned long lastEdgeTime1 = 0;
unsigned long lastEdgeTime2 = 0;

// state: 0 = waiting for next rising edge, 1 = pulse is HIGH
uint8_t stepState1 = 0;
uint8_t stepState2 = 0;

// --- Step counter (Tracks M1 cycles) ---
long stepsTaken1 = 0;


void setup() {
 // pins setup
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

 // set initial directions
 dirState1 = (currentCycleState == COILING);
 digitalWrite(dirPin1, dirState1 ? HIGH : LOW);
 digitalWrite(dirPin2, dirState2 ? HIGH : LOW);

 // set microstepping same as original
 setMicrostepping(2, ms1_1, ms2_1, ms3_1);  // Stepper 1: Half step
 setMicrostepping(4, ms1_2, ms2_2, ms3_2);  // Stepper 2: Quarter step

 // ensure step pins are LOW initially
 digitalWrite(stepPin1, LOW);
 digitalWrite(stepPin2, LOW);

 // initialize timing
 unsigned long now = micros();
 lastEdgeTime1 = now;
 lastEdgeTime2 = now;
}

void loop() {
 unsigned long now = micros();

 // ---------- Motor 1 (Coil/Return Cycle Controller) ----------
 if (stepState1 == 0) { // Waiting for next rising edge
  if (now - lastEdgeTime1 >= period1_us) {
   digitalWrite(stepPin1, HIGH);
   lastEdgeTime1 = now;
   stepState1 = 1;
  }
 } else { // step HIGH
  if (now - lastEdgeTime1 >= pulseHigh1_us) {
   digitalWrite(stepPin1, LOW);
   lastEdgeTime1 = now;
   stepState1 = 0;
      
      // Count the step and check for cycle change
      stepsTaken1++; 
      
      if (stepsTaken1 >= MAX_STEPS_1) {
          
          // 1. Reset the step counter
          stepsTaken1 = 0; 
          
          // 2. Flip the state and update direction
          if (currentCycleState == COILING) {
              currentCycleState = RETURNING;
              // Motor 1 reverses to return to start point
              dirState1 = !dirState1; 
          } else {
              currentCycleState = COILING;
              // Motor 1 reverses to start coiling again
              dirState1 = !dirState1; 
          }
          
          // 3. Update the physical DIR pin for Motor 1
          digitalWrite(dirPin1, dirState1 ? HIGH : LOW); 
      }
  }
 }


 // ---------- Motor 2 (Traverse - ON only during COILING state) ----------
 // Motor 2 stepping is conditional based on the CycleState.
 if (currentCycleState == COILING) {
  if (stepState2 == 0) {
   if (now - lastEdgeTime2 >= period2_us) {
    digitalWrite(stepPin2, HIGH);
    lastEdgeTime2 = now;
    stepState2 = 1;
   }
  } else { // step HIGH
   if (now - lastEdgeTime2 >= pulseHigh2_us) {
    digitalWrite(stepPin2, LOW);
    lastEdgeTime2 = now;
    stepState2 = 0;
   }
  }
 } else {
  // Ensure M2 step pin is LOW when in RETURNING state (stopped)
  digitalWrite(stepPin2, LOW);
 }
}


// generalized function to set microstepping pins for any stepper
void setMicrostepping(int mode, int ms1, int ms2, int ms3) {
 switch(mode) {
  case 1:  digitalWrite(ms1, LOW); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
  case 2:  digitalWrite(ms1, HIGH); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
  case 4:  digitalWrite(ms1, LOW); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break;
  case 8:  digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break;
  case 16: digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, HIGH); break;
  default: digitalWrite(ms1, LOW); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
 }
}