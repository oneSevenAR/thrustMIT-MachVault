// A4988 Dual Stepper Microstepping Test with Oscillation
// Stepper 1: STEP D3, DIR D4, MS1 D8, MS2 D9, MS3 D10
// Stepper 2: STEP D5, DIR D6, MS1 D11, MS2 D12, MS3 D13

// Stepper 1 pins
const int stepPin1 = 3;
const int dirPin1  = 4;
const int ms1_1 = 8;
const int ms2_1 = 9;
const int ms3_1 = 10;

// Stepper 2 pins
const int stepPin2 = 5;
const int dirPin2  = 6;
const int ms1_2 = 11;
const int ms2_2 = 12;
const int ms3_2 = 13;

unsigned long previousMillis = 0;
const unsigned long interval = 5000; // 2 seconds

bool stepper1Forward = true;
bool stepper2Forward = true;

void setup() {
  // Stepper 1
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(ms1_1, OUTPUT);
  pinMode(ms2_1, OUTPUT);
  pinMode(ms3_1, OUTPUT);

  // Stepper 2
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(ms1_2, OUTPUT);
  pinMode(ms2_2, OUTPUT);
  pinMode(ms3_2, OUTPUT);

  // Set initial microstepping modes
  setMicrostepping(4, ms1_1, ms2_1, ms3_1);   // Stepper 1: Half step
  setMicrostepping(4, ms1_2, ms2_2, ms3_2);   // Stepper 2: Quarter step

  // Set initial directions
  digitalWrite(dirPin1, HIGH);
  digitalWrite(dirPin2, HIGH);

  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if 2 seconds passed to toggle directions
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Toggle direction flags
    stepper1Forward = !stepper1Forward;
    stepper2Forward = !stepper2Forward;

    // Set directions for steppers
    digitalWrite(dirPin1, stepper1Forward ? HIGH : LOW);
    digitalWrite(dirPin2, stepper2Forward ? HIGH : LOW);
  }

  // Step both steppers
  stepMotor(stepPin1);
  stepMotor(stepPin2);
}

// Helper function to pulse step pin
void stepMotor(int stepPin) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(800);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(800);
}

// generalized function to set microstepping pins for any stepper
void setMicrostepping(int mode, int ms1, int ms2, int ms3) {
  switch(mode) {
    case 1:   // Full step
      digitalWrite(ms1, LOW); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
    case 2:   // Half step
      digitalWrite(ms1, HIGH); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
    case 4:   // Quarter step
      digitalWrite(ms1, LOW); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break;
    case 8:   // Eighth step
      digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break;
    case 16:  // Sixteenth step
      digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, HIGH); break;
  }
}
