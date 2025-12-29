#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedTOF.h>

m5::unit::UnitUnified Units;
m5::unit::UnitToF unit;

void setup()
{
    M5.begin();

    // M5StickC Plus2: SDA=GPIO0, SCL=GPIO26 (Port A)
    auto pin_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    Wire.begin(pin_sda, pin_scl, 400000U);

    if (!Units.add(unit, Wire) || !Units.begin())
    {
        M5.Display.clear();
        M5.Display.setTextColor(TFT_RED);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(0, 20);
        M5.Display.println("ToF NOT");
        M5.Display.println("found!");
        while (1)
        {
            delay(1000);
        }
    }

    M5.Display.clear();
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(0, 20);
    M5.Display.println("ToF Ready");
    delay(2000);
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated())
    {
        auto range = unit.range();
        if (range >= 0)
        {
            M5.Display.clear();
            M5.Display.setTextColor(TFT_WHITE);
            M5.Display.setTextSize(2);
            M5.Display.setCursor(10, 30);
            M5.Display.printf("Dist:");
            M5.Display.setCursor(10, 50);
            M5.Display.printf("%d mm", range);
        }
    }

    delay(100);
}
