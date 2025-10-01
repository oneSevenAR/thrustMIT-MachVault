// A4988 Dual Stepper Microstepping Test
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

  // Set direction for each stepper (can change as needed)
  digitalWrite(dirPin1, HIGH);
  digitalWrite(dirPin2, LOW);

  // Set microstepping mode for each
  setMicrostepping(2, ms1_1, ms2_1, ms3_1);   // Stepper 1: Half step
  setMicrostepping(4, ms1_2, ms2_2, ms3_2);   // Stepper 2: Quarter step
}

void loop() {
  // Stepper 1 stepping
  digitalWrite(stepPin1, HIGH);
  delayMicroseconds(800);
  digitalWrite(stepPin1, LOW);
  delayMicroseconds(800);

  // Stepper 2 stepping (adjust delay as needed for speed)
  digitalWrite(stepPin2, HIGH);
  delayMicroseconds(500);
  digitalWrite(stepPin2, LOW);
  delayMicroseconds(500);
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
