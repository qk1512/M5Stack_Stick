#include "tof_filter.h"
#include <algorithm>
#include <cmath>
#include <cstring>
using namespace std;

ToFFilter::ToFFilter() 
    : med_i(0)
    , buf_filled(false)
    , ema(NAN)
    , stable_mm(-1)
{
    memset(med_buf, 0, sizeof(med_buf));
}

void ToFFilter::reset() {
    med_i = 0;
    buf_filled = false;
    ema = NAN;
    stable_mm = -1;
    memset(med_buf, 0, sizeof(med_buf));
}

int ToFFilter::median5(const int *a) {
    int b[MED_N];
    memcpy(b, a, sizeof(b));
    std::sort(b, b + MED_N);
    return b[MED_N / 2];
}

int ToFFilter::process(int raw_mm, bool moving) {
    if (raw_mm < 0) return -1;            // invalid
    if (raw_mm > 2000) return -1;         // optional clamp theo sensor

    // 1) Median buffer
    med_buf[med_i] = raw_mm;
    med_i = (med_i + 1) % MED_N;
    if (med_i == 0) buf_filled = true;

    int med = buf_filled ? median5(med_buf) : raw_mm;

    // 2) EMA
    float alpha = moving ? 0.30f : 0.15f;
    if (!isfinite(ema)) ema = med;
    ema = alpha * med + (1.0f - alpha) * ema;

    // 3) Deadband
    int out = (int)lroundf(ema);
    if (stable_mm < 0) stable_mm = out;

    if (abs(out - stable_mm) >= 5) {      // deadband ±5mm
        stable_mm = out;
    }
    return stable_mm;
}
