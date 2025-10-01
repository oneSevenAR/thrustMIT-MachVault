/*
     Basic example code for controlling a stepper without library

     by Dejan, https://howtomechatronics.com
*/

// defines pins
#define stepPin 2
#define dirPin 5

int customDelay, customDelayMapped;

void setup() {
  // Sets the two pins as Outputs
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
}
void loop() {
  speedControl();
  // Makes pules with custom delay, depending on the Potentiometer, from which the speed of the motor depends
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(customDelayMapped);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(customDelayMapped);
}i
// Custom function for reading the potentiometer and mapping its value from 300 to 3000, suitable for the custom delay value in microseconds
void speedControl() {
  customDelay = analogRead(A0); // Read the potentiometer value
  customDelayMapped = map(customDelay, 0, 1023, 300, 3000); // Convert the analog input from 0 to 1024, to 300 to 3000
}