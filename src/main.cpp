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

// Task: Read IMU sensor at high frequency for motion detection
void imuTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 50ms = 20Hz
    
    for (;;) {
        // Update IMU sensor
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
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms = 10Hz
    
    for (;;) {
        // Get current motion state for adaptive filtering
        bool is_moving = false;
        if (xSemaphoreTake(imuMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            is_moving = shared_is_moving;
            xSemaphoreGive(imuMutex);
        }
        
        // Update sensor with motion-aware filtering
        tofSensor.update(is_moving);
        int distance = tofSensor.getDistance();
        
        // Update shared data with mutex protection
        if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            shared_distance = distance;
            xSemaphoreGive(distanceMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Task: Update display and handle UI interactions
void displayTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms = 10Hz
    
    for (;;) {
        // Update M5 button states
        M5.update();
        
        // Handle button press to toggle display mode
        if (M5.BtnA.wasPressed()) {
            displayManager.toggleMode();
        }
        
        // Read sensor data from shared memory
        int distance = -1;
        IMUData imu_data;
        bool is_moving = false;
        
        if (xSemaphoreTake(distanceMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            distance = shared_distance;
            xSemaphoreGive(distanceMutex);
        }
        
        if (xSemaphoreTake(imuMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            imu_data = shared_imu_data;
            is_moving = shared_is_moving;
            xSemaphoreGive(imuMutex);
        }
        
        // Update display
        displayManager.update(distance, imu_data, is_moving);
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup()
{
    // Initialize M5 system
    M5.begin();

    // Initialize display with landscape orientation
    displayManager.begin(1);

    // Get I2C pins for M5StickC Plus2 (Port A: SDA=GPIO0, SCL=GPIO26)
    auto pin_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_scl = M5.getPin(m5::pin_name_t::port_a_scl);

    // Initialize ToF sensor (critical - halt on failure)
    if (!tofSensor.begin(pin_sda, pin_scl))
    {
        displayManager.showError("ToF NOT\nfound!");
        while (1) { delay(1000); }
    }

    // Initialize IMU sensor (optional - continue on failure)
    if (!imuSensor.begin())
    {
        displayManager.showError("IMU NOT\nfound!", TFT_YELLOW);
        delay(2000);
    }

    // Show ready message
    displayManager.showStartup("Sensors\nReady");
    delay(2000);

    // Create mutexes for thread-safe data sharing
    distanceMutex = xSemaphoreCreateMutex();
    imuMutex = xSemaphoreCreateMutex();

    // Create FreeRTOS tasks on Core 1
    // IMU Task: High priority for responsive motion detection
    xTaskCreatePinnedToCore(
        imuTask,
        "IMU",
        4096,
        NULL,
        3,                    // Highest priority
        &imuTaskHandle,
        1
    );

    // ToF Sensor Task: Medium priority with motion-aware filtering
    xTaskCreatePinnedToCore(
        sensorTask,
        "ToF",
        4096,
        NULL,
        2,                    // Medium priority
        &sensorTaskHandle,
        1
    );

    // Display Task: Low priority for UI updates
    xTaskCreatePinnedToCore(
        displayTask,
        "Display",
        4096,
        NULL,
        1,                    // Lowest priority
        &displayTaskHandle,
        1
    );
}

void loop()
{
    // Empty - FreeRTOS tasks handle everything
    vTaskDelay(pdMS_TO_TICKS(1000));
}
