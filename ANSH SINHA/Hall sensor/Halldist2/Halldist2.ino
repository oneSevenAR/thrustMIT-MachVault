// Linear Hall Effect Sensor (49E M45WB) with Arduino
// Auto-calibrates baseline at startup and measures both magnetic poles.

const int hallPin = A0;
const float Vref = 5.0;
const float sensitivity = 0.001333; // V/G (1.333 mV/G)

float baselineV = 0; // Will be measured at startup

void setup() {
  Serial.begin(9600);
  delay(1000); // Allow sensor to stabilize

  // Take average of 100 readings for a more stable baseline
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(hallPin);
    delay(5);
  }
  int avgADC = sum / 100;
  baselineV = (avgADC / 1023.0) * Vref;

  // Check if the baseline voltage is within an expected range (e.g., 2.3V to 2.7V)
  if (baselineV < 2.3 || baselineV > 2.7) {
    Serial.println("Warning: Baseline voltage is outside of the expected 2.5V quiescent range.");
    Serial.print("Current baseline: ");
    Serial.print(baselineV, 3);
    Serial.println(" V");
  } else {
    Serial.print("Baseline calibrated at: ");
    Serial.print(baselineV, 3);
    Serial.println(" V");
  }
}

void loop() {
  int adc = analogRead(hallPin);
  float voltage = (adc / 1023.0) * Vref;

  // Calculate the change from the baseline voltage. This can be positive or negative.
  float deltaV = voltage - baselineV;

  // Calculate Gauss. The sign of B_gauss indicates the polarity.
  float B_gauss = deltaV / sensitivity;

  // Convert Gauss to Tesla
  float B_tesla = B_gauss * 1e-4;

  Serial.print("ADC: ");
  Serial.print(adc);
  Serial.print(" | V: ");
  Serial.print(voltage, 3);
  Serial.print(" V | ΔV: ");
  Serial.print(deltaV, 3);
  Serial.print(" V | B: ");

  // Print polarity and value
  if (B_gauss > 0.5) {
    Serial.print(B_gauss, 1);
    Serial.print(" G (South Pole)");
  } else if (B_gauss < -0.5) {
    Serial.print(B_gauss, 1);
    Serial.print(" G (North Pole)");
  } else {
    Serial.print(B_gauss, 1);
    Serial.print(" G (No significant field)");
  }
  Serial.print(" | T: ");
  Serial.println(B_tesla, 6);

  delay(100);
}