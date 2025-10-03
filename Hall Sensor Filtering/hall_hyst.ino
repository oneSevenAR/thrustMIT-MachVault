#include <SimpleKalmanFilter.h>

const int hallPin = A0;
const float Vref = 5.0;
const float sensitivity = 0.001333; // V/G

SimpleKalmanFilter kalmanFilter(1.0, 10.0, 0.1);

float baselineV = 0;      // initial baseline voltage
float idleOffset = 0;     // dynamic zero-offset
unsigned long lastOffsetUpdate = 0;

// Polarity hysteresis thresholds
const float TH_ON = 25.0;  // when to consider a positive magnetic event
const float TH_OFF = 10.0; // when to reset to zero
static bool active = false;

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initial baseline calibration (no magnet present)
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(hallPin);
    delay(2);
  }
  int avgADC = sum / 50;
  baselineV = (avgADC / 1023.0) * Vref;

  Serial.println("Raw,CorrectedKalman");
}

void loop() {
  // Read voltage and convert to Gauss
  int adc = analogRead(hallPin);
  float voltage = (adc / 1023.0) * Vref;
  float B_gauss = (voltage - baselineV) / sensitivity;

  // Kalman filter
  static bool kalmanInit = false;
  static float B_kalman = 0;

  if (!kalmanInit) {
    B_kalman = B_gauss;
    kalmanInit = true;
  } else {
    B_kalman = kalmanFilter.updateEstimate(B_gauss);
  }

  // ===== Auto-zero / slow offset correction =====
  if (abs(B_kalman) < 30.0) {  // near-zero detection
    if (millis() - lastOffsetUpdate > 2000) {
      idleOffset = 0.9 * idleOffset + 0.1 * B_kalman;
      lastOffsetUpdate = millis();
    }
  }

  float B_corrected = B_kalman - idleOffset;

  // ===== Polarity filter + hysteresis =====
  // ===== Polarity filter + hysteresis (symmetric) =====
float output = 0.0;
if (!active) {
  if (abs(B_corrected) > TH_ON) {
    active = true;
    output = B_corrected;
  } else {
    output = 0.0;
  }
} else {
  if (abs(B_corrected) > TH_OFF) output = B_corrected;
  else {
    active = false;
    output = 0.0;
  }
}


  // ===== Serial output =====
  Serial.print(B_corrected, 2); Serial.print(",");
  Serial.println(output, 2);

  delay(25); // ~13 Hz
}