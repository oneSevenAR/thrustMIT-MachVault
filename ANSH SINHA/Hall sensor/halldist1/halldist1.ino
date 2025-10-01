
const int hallSensorPin = A0;

// The neutral sensor reading when no magnet is present
const int neutralReading = 373;

// The maximum absolute difference from the neutral reading
// This is found by taking the larger of (520 - 373) or (373 - 296)
// 520 - 373 = 147
// 373 - 296 = 77
const int maxAbsReading = 147;

void setup() {
  // Start serial communication for data plotting
  Serial.begin(9600);
}

void loop() {
  // Read the raw sensor value
  int sensorReading = analogRead(hallSensorPin);

  // Calculate the absolute difference from the neutral reading.
  // This value represents the magnetic field strength, regardless of pole.
  int absReading = abs(sensorReading - neutralReading);

  // Map the absolute sensor reading to a relative distance value.
  // The mapping is inverted because a higher reading (stronger field) means a closer distance.
  // We'll map the range from 0 to maxAbsReading to 100 (far) to 0 (close).
  int mappedDistance = map(absReading, 0, maxAbsReading, 100, 0);

  // Constrain the distance value to be within 0 and 100
  mappedDistance = constrain(mappedDistance, 0, 100);

  // Print the mapped distance to the Serial Plotter
  Serial.println(mappedDistance);

  // Small delay to make the graph more readable
  delay(50);
}