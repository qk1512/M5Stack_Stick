#include "imu_sensor.h"
#include <cmath>

IMUSensor::IMUSensor() 
    : initialized(false)
    , accel_magnitude(0.0f)
{
    data.valid = false;
}

bool IMUSensor::begin() {
    // M5Unified automatically initializes IMU in M5.begin()
    // Just check if it's available
    if (M5.Imu.isEnabled()) {
        initialized = true;
        return true;
    }
    
    initialized = false;
    return false;
}

void IMUSensor::update() {
    if (!initialized) return;
    
    // Read IMU data
    if (M5.Imu.update()) {
        auto imu_data = M5.Imu.getImuData();
        
        data.accel_x = imu_data.accel.x;
        data.accel_y = imu_data.accel.y;
        data.accel_z = imu_data.accel.z;
        data.gyro_x = imu_data.gyro.x;
        data.gyro_y = imu_data.gyro.y;
        data.gyro_z = imu_data.gyro.z;
        //data.temp = imu_data.temp;
        data.valid = true;
        
        accel_magnitude = calculateAccelMagnitude();
    }
}

float IMUSensor::calculateAccelMagnitude() {
    // Calculate total acceleration magnitude
    float total = sqrtf(data.accel_x * data.accel_x + 
                       data.accel_y * data.accel_y + 
                       data.accel_z * data.accel_z);
    
    // Subtract gravity (1.0g) to get movement component
    return fabsf(total - 1.0f);
}

bool IMUSensor::isMoving(float threshold) {
    return (accel_magnitude > threshold);
}
