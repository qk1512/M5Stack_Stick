#ifndef TOF_SENSOR_H
#define TOF_SENSOR_H

#include <M5UnitUnified.h>
#include <M5UnitUnifiedTOF.h>
#include "tof_filter.h"

class ToFSensor {
public:
    ToFSensor();
    
    // Initialize sensor with I2C pins
    bool begin(int pin_sda, int pin_scl);
    
    // Update sensor readings with motion state for adaptive filtering
    void update(bool is_moving = false);
    
    // Get filtered distance in mm (-1 if invalid)
    int getDistance();
    
    // Check if sensor is ready
    bool isReady() const { return initialized; }

private:
    m5::unit::UnitUnified units;
    m5::unit::UnitToF unit;
    ToFFilter filter;
    bool initialized;
    int last_distance;
};

#endif // TOF_SENSOR_H
