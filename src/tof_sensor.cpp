#include "tof_sensor.h"
#include <Wire.h>

ToFSensor::ToFSensor() 
    : initialized(false)
    , last_distance(-1)
{
}

bool ToFSensor::begin(int pin_sda, int pin_scl) {
    Wire.begin(pin_sda, pin_scl, 400000U);
    
    if (!units.add(unit, Wire) || !units.begin()) {
        initialized = false;
        return false;
    }
    
    initialized = true;
    return true;
}

void ToFSensor::update() {
    if (!initialized) return;
    
    units.update();
    
    if (unit.updated()) {
        auto range = unit.range();
        last_distance = filter.process(range, false);
    }
}

int ToFSensor::getDistance() {
    return last_distance;
}
