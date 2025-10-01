/*
   Modified code: Stepper direction controlled by potentiometer
   Original: Dejan, https://howtomechatronics.com
*/

// Defines pins
#define stepPin 2
#define dirPin 5

int potValue;
int customDelay = 800; // Fixed speed, adjust as needed

void setup() {
  // Set pins as outputs
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
}
void loop() {
  directionControl();

  // Generate step pulse
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(customDelay);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(customDelay);
}

// Potentiometer: left of midpoint = one direction, right = other
void directionControl() {
  potValue = analogRead(A0); // Read potentiometer value (0–1023)
  if (potValue < 512) {
    digitalWrite(dirPin, LOW);  // One direction
  } else {
    digitalWrite(dirPin, HIGH); // Other direction
  }
}
