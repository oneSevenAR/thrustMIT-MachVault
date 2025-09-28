#include <SimpleKalmanFilter.h>

const int hallPin = A0;
const float Vref = 5.0;
const float sensitivity = 0.001333; // V/G

float baselineV = 0;
SimpleKalmanFilter kalmanFilter(1.0, 10.0, 0.1);  // larger Q & R

void setup() {
  Serial.begin(115200);
  delay(500);

  // Baseline calibration
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(hallPin);
    delay(2);
  }
  int avgADC = sum / 50;
  baselineV = (avgADC / 1023.0) * Vref;

  Serial.println("Raw,Kalman");
}

void loop() {
  int adc = analogRead(hallPin);
  float voltage = (adc / 1023.0) * Vref;

  float deltaV = voltage - baselineV;
  float B_gauss = deltaV / sensitivity;

  static bool kalmanInit = false;
  static float B_kalman = 0;

  if (!kalmanInit) {
    B_kalman = B_gauss;
    kalmanInit = true;
  } else {
    B_kalman = kalmanFilter.updateEstimate(B_gauss);
  }

  Serial.print(B_gauss, 2); Serial.print(",");
  Serial.println(B_kalman, 2);

  delay(75); // ~100 Hz
}
