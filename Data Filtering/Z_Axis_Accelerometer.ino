#include <Wire.h>

#define MPU9250_ADDRESS 0x68
#define ACCEL_SENSITIVITY 16384.0  // Sensitivity for ±2g range
#define GYRO_SENSITIVITY 131.0    // Sensitivity for ±250°/sec range

const float THRESHOLD_G = 1.0; // Threshold in g (1g)

// Kalman filter variables for Z axis
float kalman_z = 0; // Initial estimate
float P_z = 1.0;    // Initial estimate uncertainty
const float Q_z = 0.008; // Process noise
const float R_z = 1.4;  // Measurement noise

// Variables to store previous values
float prev_az = 0;
float prev_filtered_az = 0;

class MPU9250 {
public:
    MPU9250(uint8_t address = MPU9250_ADDRESS) : address(address) {}
    
    void begin() {
        Wire.begin();
        Wire.beginTransmission(address);
        Wire.write(0x6B); // PWR_MGMT_1 register
        Wire.write(0x00); // Wake up the MPU9250
        Wire.endTransmission();
        
        // Configure accelerometer range (±2g)
        Wire.beginTransmission(address);
        Wire.write(0x1C); // ACCEL_CONFIG register
        Wire.write(0x00); // 2g range
        Wire.endTransmission();
        
        // Configure gyroscope range (±250°/s)
        Wire.beginTransmission(address);
        Wire.write(0x1B); // GYRO_CONFIG register
        Wire.write(0x00); // 250°/s range
        Wire.endTransmission();
    }
    
    float readAccelZ() {
        int16_t raw_az;
        
        Wire.beginTransmission(address);
        Wire.write(0x3F); // ACCEL_ZOUT_H register
        Wire.endTransmission(false);
        Wire.requestFrom(address, (uint8_t)2, (uint8_t)true);
        
        raw_az = (Wire.read() << 8) | Wire.read();
        
        // Convert raw value to g
        return raw_az / ACCEL_SENSITIVITY;
    }
    
private:
    uint8_t address;
};

MPU9250 imu;

// Kalman filter implementation for a single value
float kalmanFilter(float measurement, float *state, float *P, float Q, float R) {
    // Prediction update
    // State prediction (no change in our simple model)
    // x = x
    
    // Error covariance prediction
    *P = *P + Q;
    
    // Measurement update
    float K = *P / (*P + R); // Kalman gain
    float new_state = *state + K * (measurement - *state); // State update
    *P = (1 - K) * *P; // Error covariance update
    
    *state = new_state;
    return new_state;
}

void setup() {
    Serial.begin(115200);
    imu.begin();
    delay(100); // Short delay to stabilize
    
    // Initialize previous values with first reading
    prev_az = imu.readAccelZ();
    prev_filtered_az = prev_az; // Start with unfiltered value
    
    // Print header for serial monitor (will be ignored by plotter)
    Serial.println("Raw_Z Filtered_Z Raw_Diff Filtered_Diff");
}

void loop() {
    // Read Z-axis acceleration data only
    float az = imu.readAccelZ();
    
    // Apply Kalman filter to z-axis data
    float filtered_az = kalmanFilter(az, &kalman_z, &P_z, Q_z, R_z);
    
    // Calculate differences
    float raw_diff = az - prev_az;
    float filtered_diff = filtered_az - prev_filtered_az;
    
    // First output for Serial Plotter (space-separated values)
    Serial.print(az, 4);
    Serial.print(" ");
    Serial.print(filtered_az, 4);
    Serial.print(" ");
    Serial.print(raw_diff, 4);
    Serial.print(" ");
    Serial.println(filtered_diff, 4);
    
    // Second output for Serial Monitor with more details
    if (Serial.available()) {  // Only print this if serial input is available to avoid cluttering the plotter
        Serial.print("Z-accel: ");
        Serial.print(az, 4);
        Serial.print(" g, Filtered: ");
        Serial.print(filtered_az, 4);
        Serial.print(" g");
        
        Serial.print(" | Raw diff: ");
        Serial.print(raw_diff, 4);
        Serial.print(" g, Filtered diff: ");
        Serial.print(filtered_diff, 4);
        Serial.print(" g");
        
        // Check if absolute value exceeds the threshold
        if (abs(filtered_az) > THRESHOLD_G) {
            Serial.println(" - THRESHOLD_EXCEEDED");
        } else {
            Serial.println(" - Below Threshold");
        }
    }
    
    // Update previous values
    prev_az = az;
    prev_filtered_az = filtered_az;
    
    delay(100);
}