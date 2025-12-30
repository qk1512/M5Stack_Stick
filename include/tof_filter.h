#ifndef TOF_FILTER_H
#define TOF_FILTER_H

class ToFFilter {
public:
    ToFFilter();
    
    // Process raw ToF measurement and return filtered value
    int process(int raw_mm, bool moving = false);
    
    // Reset filter state
    void reset();

private:
    static constexpr int MED_N = 5;
    int med_buf[MED_N];
    int med_i;
    bool buf_filled;
    
    float ema;
    int stable_mm;
    
    int median5(const int *a);
};

#endif // TOF_FILTER_H
