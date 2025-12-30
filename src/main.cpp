#include <M5Unified.h>
#include "tof_sensor.h"
#include "imu_sensor.h"
#include "display_manager.h"

// Global objects
ToFSensor tofSensor;
IMUSensor imuSensor;
DisplayManager displayManager;

// FreeRTOS task handles
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t imuTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;

// Shared data (protected by mutex)
SemaphoreHandle_t distanceMutex;
SemaphoreHandle_t imuMutex;
int shared_distance = -1;
IMUData shared_imu_data;
bool shared_is_moving = false;

// Task: Read IMU sensor
void imuTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms = 10Hz
    
    for (;;) {
        // Update IMU
        imuSensor.update();
        IMUData imu_data = imuSensor.getData();
        bool is_moving = imuSensor.isMoving();
        
        // Update shared data with mutex protection
        if (xSemaphoreTake(imuMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            shared_imu_data = imu_data;
            shared_is_moving = is_moving;
            xSemaphoreGive(imuMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Task: Read ToF sensor
void sensorTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 200ms = 5Hz
    
    for (;;) {
        // Update sensor
        tofSensor.update();
        int distance = tofSensor.getDistance();
        
        // Update shared data with mutex protection
        if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            shared_distance = distance;
            xSemaphoreGive(distanceMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Task: Update display
void displayTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(250); // 250ms = 4Hz
    
    for (;;) {
        M5.update();
        
        // Handle button press to toggle display mode
        if (M5.BtnA.wasPressed()) {
            displayManager.toggleMode();
        }
        
        int distance = -1;
        IMUData imu_data;
        bool is_moving = false;
        
        // Read ToF data
        if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            distance = shared_distance;
            xSemaphoreGive(distanceMutex);
        }
        
        // Read IMU data
        if (xSemaphoreTake(imuMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            imu_data = shared_imu_data;
            is_moving = shared_is_moving;
            xSemaphoreGive(imuMutex);
        }
        
        // Update display using DisplayManager
        displayManager.update(distance, imu_data, is_moving);
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup()
{
    M5.begin();

    // Initialize display manager
    displayManager.begin(1); // 1 = landscape orientation

    // M5StickC Plus2: SDA=GPIO0, SCL=GPIO26 (Port A)
    auto pin_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_scl = M5.getPin(m5::pin_name_t::port_a_scl);

    // Initialize ToF sensor
    if (!tofSensor.begin(pin_sda, pin_scl))
    {
        displayManager.showError("ToF NOT\nfound!");
        while (1)
        {
            delay(1000);
        }
    }

    // Initialize IMU sensor
    if (!imuSensor.begin())
    {
        displayManager.showError("IMU NOT\nfound!", TFT_YELLOW);
        delay(2000);
    }

    displayManager.showStartup("Sensors\nReady");
    delay(2000);

    // Create mutexes
    distanceMutex = xSemaphoreCreateMutex();
    imuMutex = xSemaphoreCreateMutex();

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        imuTask,              // Task function
        "IMUTask",            // Task name
        4096,                 // Stack size
        NULL,                 // Parameters
        3,                    // Priority (highest)
        &imuTaskHandle,       // Task handle
        1                     // Core 1
    );

    xTaskCreatePinnedToCore(
        sensorTask,           // Task function
        "SensorTask",         // Task name
        4096,                 // Stack size
        NULL,                 // Parameters
        2,                    // Priority (high)
        &sensorTaskHandle,    // Task handle
        1                     // Core 1
    );

    xTaskCreatePinnedToCore(
        displayTask,          // Task function
        "DisplayTask",        // Task name
        4096,                 // Stack size
        NULL,                 // Parameters
        1,                    // Priority (lower)
        &displayTaskHandle,   // Task handle
        1                     // Core 1
    );
}

void loop()
{
    // Empty - FreeRTOS tasks handle everything
    vTaskDelay(pdMS_TO_TICKS(1000));
}
