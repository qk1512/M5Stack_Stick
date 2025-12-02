/* #include <M5Unified.h>

// Biến giữ lại qua deep sleep
RTC_DATA_ATTR bool g_inited = false;
RTC_DATA_ATTR uint32_t g_startSec = 0;

bool isLeap(int year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// Chuyển m5::rtc_datetime_t -> số giây từ 1/1/2000
uint32_t datetimeToSeconds(const m5::rtc_datetime_t &dt)
{
    const int baseYear = 2000;
    static const int daysInMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    int year = dt.date.year;
    int month = dt.date.month;
    int day = dt.date.date;
    int hour = dt.time.hours;
    int minute = dt.time.minutes;
    int second = dt.time.seconds;

    uint32_t days = 0;

    // cộng ngày các năm trước
    for (int y = baseYear; y < year; ++y)
    {
        days += 365 + (isLeap(y) ? 1 : 0);
    }

    // cộng ngày các tháng trước trong năm hiện tại
    for (int m = 1; m < month; ++m)
    {
        days += daysInMonth[m - 1];
        if (m == 2 && isLeap(year))
        {
            days += 1;
        }
    }

    // cộng ngày trong tháng
    days += (day - 1);

    uint32_t sec = days * 24 * 3600 + hour * 3600 + minute * 60 + second;

    return sec;
}

void printHMS(uint32_t sec, int x, int y)
{
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;

    char buf[32];
    sprintf(buf, "%02lu:%02lu:%02lu",
            (unsigned long)h,
            (unsigned long)m,
            (unsigned long)s);

    M5.Display.setCursor(x, y);
    M5.Display.print(buf);
}

void goToSleep()
{
    // tắt màn
    M5.Display.setBrightness(0);

    // nút A (BtnA) trên StickC thường là GPIO37
    const gpio_num_t WAKE_BUTTON = GPIO_NUM_37;
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON, 0); // wake khi kéo LOW

    esp_deep_sleep_start();
}

//----------------------
// SETUP
//----------------------
void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);

    // lấy thời gian hiện tại từ RTC
    m5::rtc_datetime_t now = M5.Rtc.getDateTime();
    uint32_t nowSec = datetimeToSeconds(now);

    // lần đầu: lưu mốc thời gian
    if (!g_inited)
    {
        g_inited = true;
        g_startSec = nowSec;
    }

    uint32_t elapsed = nowSec - g_startSec;

    // đọc pin
    float voltage = M5.Power.getBatteryVoltage() / 1000.0f; // mV -> V
    int percent = M5.Power.getBatteryLevel();               // 0–100%

    // hiển thị
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);

    int y = 10;

    M5.Display.setCursor(10, y);
    M5.Display.print("Battery:");
    M5.Display.setCursor(10, y + 20);
    M5.Display.printf("%.2f V (%d%%)", voltage, percent);

    y += 50;
    M5.Display.setCursor(10, y);
    M5.Display.print("Uptime:");
    printHMS(elapsed, 10, y + 20);

    Serial.printf("Battery: %.2f V (%d%%), uptime: %lu s\n",
                  voltage, percent, (unsigned long)elapsed);

    // giữ 10s rồi ngủ
    delay(10000);
    goToSleep();
}

void loop()
{
    // không dùng
}
 */

#include <M5Unified.h>

constexpr int SAMPLE_INTERVAL_MS = 10; // IMU quét mỗi 10 ms
constexpr int SHOW_DURATION_MS = 5000; // 5 giây hiển thị

// Lưu các giá trị IMU gần nhất
float g_ax = 0, g_ay = 0, g_az = 0;
float g_gx = 0, g_gy = 0, g_gz = 0;

bool showScreen = false;
uint32_t lastSample = 0;
uint32_t showStart = 0;

void drawScreen()
{
    // Lấy thông tin pin
    float voltage = M5.Power.getBatteryVoltage() / 1000.0f;
    int percent = M5.Power.getBatteryLevel();

    // Clear màn hình
    M5.Display.fillScreen(BLACK);

    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);

    // Battery
    M5.Display.setCursor(10, 10);
    M5.Display.printf("Pin: %.2fV %d%%", voltage, percent);

    // IMU title
    M5.Display.setCursor(10, 40);
    M5.Display.print("IMU 10ms:");

    // ACCEL
    M5.Display.setCursor(10, 70);
    M5.Display.printf("Ax: %.2f", g_ax);
    M5.Display.setCursor(10, 90);
    M5.Display.printf("Ay: %.2f", g_ay);
    M5.Display.setCursor(10, 110);
    M5.Display.printf("Az: %.2f", g_az);

}

//---------------------------------------------------------
void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setBrightness(0); // luôn tắt màn hình khi không xem
}

//---------------------------------------------------------
void loop()
{
    M5.update();
    uint32_t now = millis();

    if (now - lastSample >= SAMPLE_INTERVAL_MS)
    {
        lastSample = now;

        M5.Imu.getAccel(&g_ax, &g_ay, &g_az);
        M5.Imu.getGyro(&g_gx, &g_gy, &g_gz);

        // Debug nếu muốn kiểm tra
        // Serial.printf("A(%.2f %.2f %.2f)\n", g_ax, g_ay, g_az);
    }

    if (M5.BtnA.wasPressed())
    {
        showScreen = true;
        showStart = now;

        M5.Display.setBrightness(80); // độ sáng màn
        drawScreen();
    }

    if (showScreen)
    {
        static uint32_t lastDraw = 0;
        if (now - lastDraw > 1000)
        { // refresh màn
            lastDraw = now;
            drawScreen();
        }

        // Hết thời gian → tắt màn
        if (now - showStart > SHOW_DURATION_MS)
        {
            showScreen = false;
            M5.Display.setBrightness(0);
            M5.Display.fillScreen(BLACK);
        }
    }

}
