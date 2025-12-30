#ifndef IMU_SENSOR_H
#define IMU_SENSOR_H

#include <M5Unified.h>

struct IMUData {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temp;
    bool valid;
};

class IMUSensor {
public:
    IMUSensor();
    
    // Initialize IMU
    bool begin();
    
    // Update IMU readings
    void update();
    
    // Get latest IMU data
    IMUData getData() const { return data; }
    
    // Check if device is moving (based on acceleration magnitude)
    bool isMoving(float threshold = 0.15f);
    
    // Check if sensor is ready
    bool isReady() const { return initialized; }

private:
    bool initialized;
    IMUData data;
    float accel_magnitude;
    
    // Calculate acceleration magnitude (removing gravity)
    float calculateAccelMagnitude();
};

#endif // IMU_SENSOR_H
