#include <PID_v1_bc.h>

// Pin setup
const int IN1 = 9;
const int IN2 = 10;
const int ENA = 6;
const int hallPin = A0;

// PID variables
double setpoint = 700;   // target hall reading for levitation
double input;            // current hall sensor reading
double output;           // pwm output from PID (0–255)

// PID tuning constants (tune for stability)
double Kp = 2.25, Ki = 0.0, Kd = 0.0;

// create PID object
PID levitatePID(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(hallPin, INPUT);
  
  digitalWrite(IN1, HIGH);  // fixed polarity
  digitalWrite(IN2, LOW);

  Serial.begin(9600);
  Serial.println("PID Magnetic Levitation Control + Serial Plotter");

  levitatePID.SetMode(AUTOMATIC);
  levitatePID.SetOutputLimits(0, 255);  // PWM range
  TCCR0B = TCCR0B & B11111000 | B00000010; //  7812.50 Hz
}

void loop() {
  // read hall sensor
  input = analogRead(hallPin);

  // compute PID output
  levitatePID.Compute();

  // write PWM to electromagnet
  analogWrite(ENA, output);

  // serial plotter format: hallValue, setpoint, pwmOutput
  Serial.print(input);
  Serial.print(",");
  Serial.print(setpoint);
  Serial.print(",");
  Serial.println(output);

  // delay(10); // short delay for smoother plotting
}
