#include <Wire.h>
#include <math.h>
#include <Adafruit_PWMServoDriver.h>

// --- PCA9685 Configuration ---
// The default I2C address for the PCA9685 is 0x40.
// If you have multiple boards, you might need to change this.
#define PCA9685_ADDRESS 0x40
#define SERVO_CHANNEL 0 // The channel on the PCA9685 board your servo is connected to (0-15)

// --- MPU9250 Configuration ---
#define MPU9250_ADDRESS 0x68
#define ACCEL_RANGE_2G 16384.0
#define GYRO_RANGE_250DPS 131.0

// --- Filter Parameters ---
const float COMP_FILTER_ALPHA = 0.98;

// --- Smoothing Parameters for Servo Control ---
const float EMA_ALPHA = 0.1;
const float DEADBAND_THRESHOLD = 2.0;

// --- Servo Angle to Pulse Width Mapping ---
// These values are for a standard servo. You may need to
// fine-tune these to match your specific servo for its full range.
#define SERVO_MIN_PULSE_US 500  // Minimum pulse width in microseconds (usually 500-600)
#define SERVO_MAX_PULSE_US 2500 // Maximum pulse width in microseconds (usually 2400-2500)
#define FREQUENCY 50 // Servo PWM frequency in Hz (standard is 50Hz)

// --- Global Variables ---
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDRESS);
float pitch = 0.0;
float smoothed_pitch = 0.0;
float last_servo_write_angle = 0.0;
unsigned long lastMicros = 0;

// --- Function to convert microsecond pulse to PCA9685 12-bit value ---
int microToPwmValue(int micro) {
  float cycle_us = 1000000.0 / FREQUENCY;
  return (4096.0 * micro) / cycle_us;
}

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  // Initialize MPU9250
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0x00); // Wake up the MPU9250
  Wire.endTransmission(true);

  // Initialize PCA9685 and set frequency
  pwm.begin();
  pwm.setPWMFreq(FREQUENCY);
  
  lastMicros = micros();
  Serial.println("MPU9250, PCA9685, and Servo Initialized.");
  Serial.println("Smoothed Pitch (°), Servo Angle (°) ");
}

// --- Loop Function ---
void loop() {
  unsigned long currentMicros = micros();
  float dt = (float)(currentMicros - lastMicros) / 1000000.0f;
  lastMicros = currentMicros;

  // Read raw MPU9250 data
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU9250_ADDRESS, (uint8_t)6, (uint8_t)true);
  int16_t raw_ax = (Wire.read() << 8) | Wire.read();
  int16_t raw_ay = (Wire.read() << 8) | Wire.read();
  int16_t raw_az = (Wire.read() << 8) | Wire.read();

  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU9250_ADDRESS, (uint8_t)6, (uint8_t)true);
  int16_t raw_gx = (Wire.read() << 8) | Wire.read();
  int16_t raw_gy = (Wire.read() << 8) | Wire.read();
  int16_t raw_gz = (Wire.read() << 8) | Wire.read();

  // Convert raw values to degrees/second and g's
  float gyro_pitch_deg_s = (float)raw_gy / GYRO_RANGE_250DPS;
  float accel_x_g = (float)raw_ax / ACCEL_RANGE_2G;
  float accel_y_g = (float)raw_ay / ACCEL_RANGE_2G;
  float accel_z_g = (float)raw_az / ACCEL_RANGE_2G;
  
  // Calculate pitch from accelerometer
  float accel_pitch = atan2(accel_x_g, sqrt(accel_y_g * accel_y_g + accel_z_g * accel_z_g)) * 180.0 / M_PI;

  // Apply Complementary Filter
  pitch = COMP_FILTER_ALPHA * (pitch + gyro_pitch_deg_s * dt) + (1.0 - COMP_FILTER_ALPHA) * accel_pitch;

  // Apply EMA filter for smoothing
  smoothed_pitch = EMA_ALPHA * pitch + (1 - EMA_ALPHA) * smoothed_pitch;

  // Map the smoothed pitch to a pulse width value for the servo
  float target_angle = constrain(smoothed_pitch, -50, 120);
  int pulse_us = map(target_angle, -50, 120, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
  
  // Convert pulse width to PCA9685 12-bit value
  int servo_pwm_value = microToPwmValue(pulse_us);

  // Apply deadband
  if (abs(target_angle - last_servo_write_angle) > DEADBAND_THRESHOLD) {
    pwm.setPWM(SERVO_CHANNEL, 0, servo_pwm_value);
    last_servo_write_angle = target_angle;
  }

  // Print to Serial Monitor
  Serial.print(smoothed_pitch, 2);
  Serial.print(",");
  Serial.println(last_servo_write_angle);
  
  delay(10);
}
