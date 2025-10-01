// A4988 Microstepping Test
// STEP pin: D3
// DIR pin: D4
// MS1, MS2, MS3: D8, D9, D10

const int stepPin = 3;
const int dirPin  = 4;
const int ms1 = 8;
const int ms2 = 9;
const int ms3 = 10;

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(ms1, OUTPUT);
  pinMode(ms2, OUTPUT);
  pinMode(ms3, OUTPUT);

  digitalWrite(dirPin, HIGH); // choose direction
  setMicrostepping(16);       // try 1, 2, 4, 8, or 16 microsteps
}

void loop() {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(800);   // adjust speed (lower = faster)
  digitalWrite(stepPin, LOW);
  delayMicroseconds(800);
}

// function to set microstepping mode
void setMicrostepping(int mode) {
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
