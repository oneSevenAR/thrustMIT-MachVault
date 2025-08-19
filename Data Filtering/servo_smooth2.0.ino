#include <Wire.h> // Required for I2C communication with MPU9250
#include <math.h> // Required for mathematical functions like atan2, sqrt, sin, cos, asin
#include <Servo.h> // Include the Servo library

// --- Servo Configuration ---
#define SERVO_PIN 9 // The digital pin the servo is connected to
const float SERVO_MIN_ANGLE = -40; // The minimum angle for the servo motion
const float SERVO_MAX_ANGLE = 170; // The maximum angle for the servo motion

// --- MPU9250 Configuration ---
#define MPU9250_ADDRESS 0x68 // I2C address of the MPU9250
#define ACCEL_RANGE_2G 16384.0 // Sensitivity for ±2g range (LSB/g)
#define GYRO_RANGE_250DPS 131.0 // Sensitivity for ±250°/s range (LSB/(°/s))

// --- Filter Parameters ---
const float COMP_FILTER_ALPHA = 0.98; // Complementary filter alpha (0 to 1, closer to 1 trusts gyro more)
const float MADGWICK_FILTER_BETA = 0.033; // Madgwick filter beta (controls accelerometer influence)

// How often to update Madgwick and use its output to correct Complementary Filter
const int MADGWICK_UPDATE_INTERVAL_ITERATIONS = 10;

// --- Smoothing Parameters for Servo Control ---
// A lower EMA_ALPHA gives more smoothing. Try values from 0.05 to 0.3.
const float EMA_ALPHA = 0.1;
// The angle threshold (in degrees) to ignore small movements.
const float DEADBAND_THRESHOLD = 2.0;

// --- Global Variables for Smoothing ---
float smoothed_pitch = 0.0;
float last_servo_write_angle = 0.0;

// --- Data Structures for Vectors and Quaternions ---
struct Vector3 {
    float x, y, z;
};

struct Quaternion {
    float w, x, y, z;
};

// --- Global Sensor Data and Baselines ---
Vector3 accel_raw;
Vector3 gyro_raw;
Vector3 accel_baseline = {0.0, 0.0, 1.0};
Vector3 gyro_baseline = {0.0, 0.0, 0.0};

// --- MPU9250 Class ---
class MPU9250 {
public:
    uint8_t address;
    MPU9250(uint8_t addr = MPU9250_ADDRESS) : address(addr) {}

    void begin() {
        Wire.begin();
        Wire.setClock(400000);
        Wire.beginTransmission(address);
        Wire.write(0x6B);
        Wire.write(0x00);
        Wire.endTransmission(true);

        Wire.beginTransmission(address);
        Wire.write(0x1C);
        Wire.write(0x00);
        Wire.endTransmission(true);
        
        Wire.beginTransmission(address);
        Wire.write(0x1B);
        Wire.write(0x00);
        Wire.endTransmission(true);

        Serial.println("MPU9250 initialized.");
    }

    Vector3 readAccel() {
        Wire.beginTransmission(address);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(address, (uint8_t)6, (uint8_t)true);

        int16_t raw_ax = (Wire.read() << 8) | Wire.read();
        int16_t raw_ay = (Wire.read() << 8) | Wire.read();
        int16_t raw_az = (Wire.read() << 8) | Wire.read();
        
        Vector3 accel_g;
        accel_g.x = (float)raw_ax / ACCEL_RANGE_2G;
        accel_g.y = (float)raw_ay / ACCEL_RANGE_2G;
        accel_g.z = (float)raw_az / ACCEL_RANGE_2G;
        return accel_g;
    }

    Vector3 readGyro() {
        Wire.beginTransmission(address);
        Wire.write(0x43);
        Wire.endTransmission(false);
        Wire.requestFrom(address, (uint8_t)6, (uint8_t)true);

        int16_t raw_gx = (Wire.read() << 8) | Wire.read();
        int16_t raw_gy = (Wire.read() << 8) | Wire.read();
        int16_t raw_gz = (Wire.read() << 8) | Wire.read();
        
        Vector3 gyro_rad_s;
        gyro_rad_s.x = ((float)raw_gx / GYRO_RANGE_250DPS) * (M_PI / 180.0);
        gyro_rad_s.y = ((float)raw_gy / GYRO_RANGE_250DPS) * (M_PI / 180.0);
        gyro_rad_s.z = ((float)raw_gz / GYRO_RANGE_250DPS) * (M_PI / 180.0);
        return gyro_rad_s;
    }
};

MPU9250 imu;
Servo myServo;

// --- Quaternion Math Helpers (C++ Port) ---
float vectorMagnitude(Vector3 v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 vectorNormalize(Vector3 v) {
    float mag = vectorMagnitude(v);
    if (mag == 0.0f) return {0.0f, 0.0f, 0.0f};
    return {v.x / mag, v.y / mag, v.z / mag};
}

Quaternion quaternionMultiply(Quaternion q1, Quaternion q2) {
    Quaternion result;
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    return result;
}

float quaternionMagnitude(Quaternion q) {
    return sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

Quaternion quaternionNormalize(Quaternion q) {
    float mag = quaternionMagnitude(q);
    if (mag == 0.0f) return {1.0f, 0.0f, 0.0f, 0.0f};
    return {q.w / mag, q.x / mag, q.y / mag, q.z / mag};
}

Vector3 quaternionToEulerAngles(Quaternion q) {
    Vector3 angles;
    float qw = q.w, qx = q.x, qy = q.y, qz = q.z;
    
    float sinr_cosp = 2 * (qw * qx + qy * qz);
    float cosr_cosp = 1 - 2 * (qx * qx + qy * qy);
    angles.x = atan2(sinr_cosp, cosr_cosp);
    
    float sinp = 2 * (qw * qy - qz * qx);
    if (fabs(sinp) >= 1) {
        angles.y = copysign(M_PI / 2, sinp);
    } else {
        angles.y = asin(sinp);
    }
    
    float siny_cosp = 2 * (qw * qz + qx * qy);
    float cosy_cosp = 1 - 2 * (qy * qy + qz * qz);
    angles.z = atan2(siny_cosp, cosy_cosp);
    
    angles.x = angles.x * 180.0 / M_PI;
    angles.y = angles.y * 180.0 / M_PI;
    angles.z = angles.z * 180.0 / M_PI;

    return angles;
}

// --- Madgwick Filter Class ---
class MadgwickFilter {
public:
    float beta;
    Quaternion q;

    MadgwickFilter(float b = MADGWICK_FILTER_BETA) : beta(b) {
        q = {1.0f, 0.0f, 0.0f, 0.0f};
        Serial.println("Madgwick filter initialized");
    }

    void update(Vector3 gyro_rad_s, Vector3 accel_g, float dt) {
        Quaternion q_norm = quaternionNormalize(q);
        Vector3 accel_norm = vectorNormalize(accel_g);

        float f[3];
        f[0] = 2*(q_norm.x*q_norm.z - q_norm.w*q_norm.y) - accel_norm.x;
        f[1] = 2*(q_norm.w*q_norm.x + q_norm.y*q_norm.z) - accel_norm.y;
        f[2] = 2*(0.5f - q_norm.x*q_norm.x - q_norm.y*q_norm.y) - accel_norm.z;

        float J[3][4];
        J[0][0] = -2*q_norm.y; J[0][1] = 2*q_norm.z; J[0][2] = -2*q_norm.w; J[0][3] = 2*q_norm.x;
        J[1][0] = 2*q_norm.x;  J[1][1] = 2*q_norm.w;
        J[1][2] = 2*q_norm.z;  J[1][3] = 2*q_norm.y;
        J[2][0] = 0;           J[2][1] = -4*q_norm.x; J[2][2] = -4*q_norm.y; J[2][3] = 0;
        
        float gradient[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                gradient[i] += J[j][i] * f[j];
            }
        }
        float grad_mag = sqrt(gradient[0]*gradient[0] + gradient[1]*gradient[1] + gradient[2]*gradient[2] + gradient[3]*gradient[3]);
        if (grad_mag != 0.0f) {
            gradient[0] /= grad_mag;
            gradient[1] /= grad_mag;
            gradient[2] /= grad_mag; gradient[3] /= grad_mag;
        }

        Quaternion gyro_q_rate = {0, gyro_rad_s.x, gyro_rad_s.y, gyro_rad_s.z};
        Quaternion q_dot_gyro = quaternionMultiply(q_norm, gyro_q_rate);
        q_dot_gyro.w *= 0.5f; q_dot_gyro.x *= 0.5f; q_dot_gyro.y *= 0.5f; q_dot_gyro.z *= 0.5f;
        
        Quaternion q_dot_final;
        q_dot_final.w = q_dot_gyro.w - beta * gradient[0];
        q_dot_final.x = q_dot_gyro.x - beta * gradient[1];
        q_dot_final.y = q_dot_gyro.y - beta * gradient[2];
        q_dot_final.z = q_dot_gyro.z - beta * gradient[3];
        
        q.w += q_dot_final.w * dt;
        q.x += q_dot_final.x * dt;
        q.y += q_dot_final.y * dt;
        q.z += q_dot_final.z * dt;

        q = quaternionNormalize(q);
    }
};

// --- Complementary Filter Class ---
class ComplementaryFilter {
public:
    float alpha;
    float pitch;
    float roll;

    ComplementaryFilter(float a = COMP_FILTER_ALPHA) : alpha(a) {
        pitch = 0.0f;
        roll = 0.0f;
        Serial.println("Complementary filter initialized.");
    }

    void update(Vector3 gyro_rad_s, Vector3 accel_g, float dt) {
        float gyro_pitch_change = gyro_rad_s.y * dt;
        float gyro_roll_change = gyro_rad_s.x * dt;

        pitch += gyro_pitch_change;
        roll += gyro_roll_change;
        
        float accel_pitch, accel_roll;
        if (accel_g.z == 0.0f && accel_g.y == 0.0f) {
            accel_pitch = 0.0f;
            accel_roll = 0.0f;
        } else {
            accel_pitch = atan2(accel_g.x, sqrt(accel_g.y * accel_g.y + accel_g.z * accel_g.z));
            accel_roll = atan2(accel_g.y, sqrt(accel_g.x * accel_g.x + accel_g.z * accel_g.z));
        }

        pitch = alpha * pitch + (1.0f - alpha) * accel_pitch;
        roll = alpha * roll + (1.0f - alpha) * accel_roll;
    }

    float getPitchDegrees() { return pitch * 180.0f / M_PI; }
    float getRollDegrees() { return roll * 180.0f / M_PI; }
};

// --- Global Filter Objects ---
MadgwickFilter madgwickFilter(MADGWICK_FILTER_BETA);
ComplementaryFilter complementaryFilter(COMP_FILTER_ALPHA);

// --- Time Tracking for dt Calculation ---
unsigned long lastMicros = 0;
int iteration_count = 0;

// --- Setup Function (runs once) ---
void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    imu.begin();
    delay(100);
    
    myServo.attach(SERVO_PIN);
    
    Serial.println("Calibrating MPU9250...");
    Vector3 temp_accel_sum = {0.0f, 0.0f, 0.0f};
    Vector3 temp_gyro_sum = {0.0f, 0.0f, 0.0f};
    int calibration_samples = 100;
    for (int i = 0; i < calibration_samples; i++) {
        Vector3 a = imu.readAccel();
        Vector3 g = imu.readGyro();
        temp_accel_sum.x += a.x; temp_accel_sum.y += a.y; temp_accel_sum.z += a.z;
        temp_gyro_sum.x += g.x; temp_gyro_sum.y += g.y;
        temp_gyro_sum.z += g.z;
        delay(10);
    }
    accel_baseline.x = temp_accel_sum.x / calibration_samples;
    accel_baseline.y = temp_accel_sum.y / calibration_samples;
    accel_baseline.z = temp_accel_sum.z / calibration_samples;
    gyro_baseline.x = temp_gyro_sum.x / calibration_samples;
    gyro_baseline.y = temp_gyro_sum.y / calibration_samples;
    gyro_baseline.z = temp_gyro_sum.z / calibration_samples;

    Serial.print("Calibration Complete. Accel Baseline (X,Y,Z): ");
    Serial.print(accel_baseline.x, 4); Serial.print(", ");
    Serial.print(accel_baseline.y, 4); Serial.print(", ");
    Serial.println(accel_baseline.z, 4);
    Serial.print("Gyro Baseline (X,Y,Z): ");
    Serial.print(gyro_baseline.x, 4); Serial.print(", ");
    Serial.print(gyro_baseline.y, 4); Serial.print(", ");
    Serial.println(gyro_baseline.z, 4);

    lastMicros = micros();
    Serial.println("Timestamp,Comp_Pitch_Deg,Comp_Roll_Deg,Madg_Roll_Deg,Madg_Pitch_Deg,Madg_Yaw_Deg,Correction_Applied");
}

// --- Loop Function (runs repeatedly) ---
void loop() {
    unsigned long currentMicros = micros();
    float dt = (float)(currentMicros - lastMicros) / 1000000.0f;
    lastMicros = currentMicros;

    accel_raw = imu.readAccel();
    gyro_raw = imu.readGyro();
    
    Vector3 calibrated_accel;
    calibrated_accel.x = accel_raw.x - accel_baseline.x;
    calibrated_accel.y = accel_raw.y - accel_baseline.y;
    calibrated_accel.z = accel_raw.z - (accel_baseline.z - 1.0f);

    Vector3 calibrated_gyro;
    calibrated_gyro.x = gyro_raw.x - gyro_baseline.x;
    calibrated_gyro.y = gyro_raw.y - gyro_baseline.y;
    calibrated_gyro.z = gyro_raw.z - gyro_baseline.z;

    complementaryFilter.update(calibrated_gyro, calibrated_accel, dt);
    
    iteration_count++;
    if (iteration_count >= MADGWICK_UPDATE_INTERVAL_ITERATIONS) {
        madgwickFilter.update(calibrated_gyro, calibrated_accel, dt);
        Vector3 madg_euler = quaternionToEulerAngles(madgwickFilter.q);
        
        complementaryFilter.pitch = madg_euler.y * M_PI / 180.0f;
        complementaryFilter.roll = madg_euler.x * M_PI / 180.0f;
        
        iteration_count = 0;
    }
    
    // Get the final angle in degrees
    float comp_pitch_deg = complementaryFilter.getPitchDegrees();
    
    // Apply EMA filter for smoothing
    smoothed_pitch = EMA_ALPHA * comp_pitch_deg + (1 - EMA_ALPHA) * smoothed_pitch;

    // Control the servo based on the smoothed pitch angle with a deadband
    float target_angle = constrain(smoothed_pitch, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    int servo_write_angle = (int)((target_angle - SERVO_MIN_ANGLE) * 180.0 / (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE));

    if (abs(servo_write_angle - last_servo_write_angle) > DEADBAND_THRESHOLD) {
        myServo.write(servo_write_angle);
        last_servo_write_angle = servo_write_angle;
    }

    // Print the final angles to the serial monitor for debugging
    Serial.print("Smoothed Pitch: ");
    Serial.print(smoothed_pitch, 2);
    Serial.print("° Servo Angle: ");
    Serial.print(last_servo_write_angle);
    Serial.println("°");
    
    delay(100);
}
