#include "display_manager.h"

// ===== helper =====
static float accMag(float ax, float ay, float az) {
  return sqrtf(ax*ax + ay*ay + az*az);
}
static float gyroMag(float gx, float gy, float gz) {
  return sqrtf(gx*gx + gy*gy + gz*gz);
}

DisplayManager::DisplayManager() 
    : current_mode(DisplayMode::SPORT)
{
}

void DisplayManager::begin(uint8_t rotation) {
    M5.Display.setRotation(rotation);
    M5.Display.setBrightness(128);
}

void DisplayManager::showStartup(const char* message, uint32_t color) {
    M5.Display.clear();
    M5.Display.setTextColor(color);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(10, 20);
    M5.Display.println(message);
}

void DisplayManager::showError(const char* message, uint32_t color) {
    M5.Display.clear();
    M5.Display.setTextColor(color);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(5, 15);
    M5.Display.println(message);
}

void DisplayManager::update(int distance, const IMUData& imu_data, bool is_moving) {
    switch (current_mode) {
        case DisplayMode::COMPACT:
            displayCompact(distance, imu_data, is_moving);
            break;
        case DisplayMode::SPORT:
            displaySport(distance, imu_data, is_moving);
            break;
        case DisplayMode::DISTANCE_ONLY:
            displayDistanceOnly(distance, is_moving);
            break;
    }
}

void DisplayManager::toggleMode() {
    switch (current_mode) {
        case DisplayMode::COMPACT:
            current_mode = DisplayMode::SPORT;
            break;
        case DisplayMode::SPORT:
            current_mode = DisplayMode::DISTANCE_ONLY;
            break;
        case DisplayMode::DISTANCE_ONLY:
            current_mode = DisplayMode::COMPACT;
            break;
    }
}

void DisplayManager::displayCompact(int distance, const IMUData& imu_data, bool is_moving) {
    M5.Display.clear();
    M5.Display.setTextSize(1);
    
    // Distance
    if (distance >= 0) {
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setCursor(5, 5);
        M5.Display.printf("Dist: %dmm", distance);
    }
    
    // Status indicator
    M5.Display.setTextColor(is_moving ? TFT_RED : TFT_GREEN);
    M5.Display.setCursor(5, 20);
    M5.Display.printf("%s", is_moving ? "MOVING" : "STILL");
    
    // IMU data
    if (imu_data.valid) {
        M5.Display.setTextColor(is_moving ? TFT_RED : TFT_GREEN);
        M5.Display.setCursor(5, 35);
        M5.Display.printf("Acc: %.2f %.2f %.2f", 
            imu_data.accel_x, imu_data.accel_y, imu_data.accel_z);
        
        M5.Display.setCursor(5, 50);
        M5.Display.printf("Gyro:%.0f %.0f %.0f", 
            imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z);
    }
}

void DisplayManager::displaySport(int distance, const IMUData& imu_data, bool is_moving) {
    // ---- redraw gating (giảm nhấp nháy) ----
    static int last_dist = -9999;
    static bool last_mov = false;
    static int last_bucket = -1;   // mức "cường độ" chuyển động
    static uint32_t last_draw_ms = 0;

    // tính intensity dựa IMU (nếu valid) hoặc fallback
    float amag = imu_data.valid ? accMag(imu_data.accel_x, imu_data.accel_y, imu_data.accel_z) : 1.0f;
    float gmag = imu_data.valid ? gyroMag(imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z) : 0.0f;

    // bucket 0..4 (cột "bars")
    int bucket = 0;
    if (imu_data.valid) {
        // amag ~ 1g khi đứng yên. Lấy độ lệch khỏi 1g làm "move"
        float a_dev = fabsf(amag - 1.0f);
        if      (a_dev < 0.03f) bucket = 0;
        else if (a_dev < 0.08f) bucket = 1;
        else if (a_dev < 0.15f) bucket = 2;
        else if (a_dev < 0.25f) bucket = 3;
        else                    bucket = 4;
    } else {
        bucket = is_moving ? 3 : 0;
    }

    // chỉ vẽ lại khi có thay đổi đáng kể hoặc sau 250ms (đỡ nhấp nháy)
    bool need = false;
    if (abs(distance - last_dist) >= 2) need = true;
    if (is_moving != last_mov) need = true;
    if (bucket != last_bucket) need = true;
    if (millis() - last_draw_ms > 250) need = true;
    if (!need) return;

    last_dist = distance;
    last_mov = is_moving;
    last_bucket = bucket;
    last_draw_ms = millis();

    // ---- colors ----
    uint16_t accent = is_moving ? TFT_ORANGE : TFT_CYAN; // sport vibe
    uint16_t bg = TFT_BLACK;

    M5.Display.clear(bg);

    // =========================
    // HEADER: status + bars
    // =========================
    // status pill
    int pill_x = 6, pill_y = 6, pill_w = 78, pill_h = 18, r = 8;
    M5.Display.fillRoundRect(pill_x, pill_y, pill_w, pill_h, r, accent);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.setCursor(pill_x + 10, pill_y + 5);
    M5.Display.print(is_moving ? "MOVING" : "REST");

    // signal bars (cường độ chuyển động)
    int bars_x = 92, bars_y = 8;
    for (int i = 0; i < 4; i++) {
        int h = 4 + i * 3;               // 4,7,10,13
        int w = 5;
        int gap = 3;
        int x = bars_x + i * (w + gap);
        int y = bars_y + (14 - h);
        uint16_t c = (i <= bucket-1) ? accent : TFT_DARKGREY;
        M5.Display.fillRoundRect(x, y, w, h, 2, c);
    }

    // divider line
    M5.Display.drawFastHLine(0, 28, M5.Display.width(), TFT_DARKGREY);

    // =========================
    // MAIN: Distance big
    // =========================
    if (distance >= 0) {
        M5.Display.setTextColor(TFT_WHITE);
        M5.Display.setTextSize(4);
        // canh giữa tương đối
        int x = 10;
        int y = 38;
        M5.Display.setCursor(x, y);
        M5.Display.printf("%d", distance);

        M5.Display.setTextSize(2);
        M5.Display.setTextColor(TFT_LIGHTGREY);
        M5.Display.setCursor(92, 60);
        M5.Display.print("mm");
    } else {
        M5.Display.setTextColor(TFT_RED);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(12, 48);
        M5.Display.print("NO RANGE");
    }

    // =========================
    // FOOTER: IMU summary
    // =========================
    M5.Display.drawFastHLine(0, 95, M5.Display.width(), TFT_DARKGREY);

    M5.Display.setTextSize(1);
    if (imu_data.valid) {
        // Acc magnitude + Gyro magnitude (sport-friendly)
        M5.Display.setTextColor(TFT_LIGHTGREY);
        M5.Display.setCursor(6, 103);
        M5.Display.printf("A:%.2fg", amag);

        M5.Display.setCursor(68, 103);
        M5.Display.printf("G:%.0fdps", gmag);
    } else {
        M5.Display.setTextColor(TFT_DARKGREY);
        M5.Display.setCursor(6, 103);
        M5.Display.print("IMU: N/A");
    }

    // "pulse" khi moving (nhịp thể thao)
    if (is_moving) {
        bool on = ((millis() / 250) % 2) == 0;
        if (on) M5.Display.fillCircle(120, 17, 3, TFT_WHITE);
    }
}

void DisplayManager::displayDistanceOnly(int distance, bool is_moving) {
    M5.Display.clear();
    
    if (distance >= 0) {
        // Large distance display
        M5.Display.setTextColor(is_moving ? TFT_RED : TFT_GREEN);
        M5.Display.setTextSize(3);
        M5.Display.setCursor(20, 20);
        M5.Display.printf("%d", distance);
        
        M5.Display.setTextSize(2);
        M5.Display.setCursor(120, 30);
        M5.Display.printf("mm");
        
        // Small status indicator
        M5.Display.setTextSize(1);
        M5.Display.setCursor(180, 70);
        M5.Display.printf(is_moving ? "MOV" : "---");
    } else {
        M5.Display.setTextColor(TFT_RED);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(40, 30);
        M5.Display.println("NO DATA");
    }
}
