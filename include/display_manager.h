#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Unified.h>
#include "imu_sensor.h"

enum class DisplayMode {
    COMPACT,      // Compact view with essential info
    SPORT,        // Sport-style UI with intensity bars
    DISTANCE_ONLY // Only show distance
};

class DisplayManager {
public:
    DisplayManager();
    
    // Initialize display with orientation
    void begin(uint8_t rotation = 1);
    
    // Display startup messages
    void showStartup(const char* message, uint32_t color = TFT_GREEN);
    void showError(const char* message, uint32_t color = TFT_RED);
    
    // Update display with sensor data
    void update(int distance, const IMUData& imu_data, bool is_moving);
    
    // Change display mode
    void setMode(DisplayMode mode) { current_mode = mode; }
    DisplayMode getMode() const { return current_mode; }
    
    // Toggle between modes
    void toggleMode();

private:
    DisplayMode current_mode;
    
    // Different display layouts
    void displayCompact(int distance, const IMUData& imu_data, bool is_moving);
    void displaySport(int distance, const IMUData& imu_data, bool is_moving);
    void displayDistanceOnly(int distance, bool is_moving);
};

#endif // DISPLAY_MANAGER_H
