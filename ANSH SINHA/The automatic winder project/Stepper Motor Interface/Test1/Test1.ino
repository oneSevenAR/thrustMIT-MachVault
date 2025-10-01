const int stepPin1 = 3;
const int dirPin1  = 4;
const int ms1_1 = 8;
const int ms2_1 = 9;
const int ms3_1 = 10;

const int stepPin2 = 5;
const int dirPin2  = 6;
const int ms1_2 = 11;
const int ms2_2 = 12;
const int ms3_2 = 13;

unsigned long previousMillis = 0;
const unsigned long interval = 5000; // 5 seconds

bool stepper1Forward = false;

void setup() {
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

  setMicrostepping(4, ms1_1, ms2_1, ms3_1);
  setMicrostepping(4, ms1_2, ms2_2, ms3_2);

  digitalWrite(dirPin1, LOW);  // initial direction for stepper1

  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // Toggle stepper1 direction every interval
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    stepper1Forward = !stepper1Forward;
    digitalWrite(dirPin1, stepper1Forward ? HIGH : LOW);
  }

  // Read potentiometer on A0
  int potValue = analogRead(A0);

  // Set stepper2 direction based on pot value (threshold 512)
  if (potValue >= 512) {
    digitalWrite(dirPin2, HIGH);
  } else {
    digitalWrite(dirPin2, LOW);
  }

  // Step motors
  stepMotor(stepPin1);
  stepMotor(stepPin2);
}

void stepMotor(int stepPin) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(200);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(200);
}

void setMicrostepping(int mode, int ms1, int ms2, int ms3) {
  switch(mode) {
    case 1:
      digitalWrite(ms1, LOW); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
    case 2:
      digitalWrite(ms1, HIGH); digitalWrite(ms2, LOW); digitalWrite(ms3, LOW); break;
    case 4:
      digitalWrite(ms1, LOW); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break;
    case 8:
      digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW); break;
    case 16:
      digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, HIGH); break;
  }
}
