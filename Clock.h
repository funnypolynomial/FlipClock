#pragma once

class Clock
{
  public:
    void DrawBackground();
    void DrawDate();
    void DrawTime();
    void CheckUpdate();
    
    const char* GetMonthName(byte Month);
    const char* GetDayName(byte Day);
    char* GetNumStr(byte Num, char* Buffer, bool noLeadingZero);

    void getTimeDigits(int* digits);
    
  private:
    void BigDigit(int x, int y, int digit1, int step, int digit2 = 0);
    void updateTime();
    void updateDate();
    int  BigDigitY();

    int currentDigits[4];
    int currentMinute = -1;
    int currentDate = -1;

    // digit layout
    int hemiDigitYGap = 2;
    int lowerYOffset = LargeDigits::HalfHeight + hemiDigitYGap;
    int midY = 320-2.4*LargeDigits::HalfHeight;
    int digitOriginX = 5;
    int interDigitX = LargeDigits::Width + 10;

    unsigned long checkTimeMS = 0;
    const unsigned long checkTimeDuration = 1000L;  // only check time every 20s, reduce glitches?, time jumps to 01:53?? reprogramming interupts I2C?
    bool blinkOn = false;
    unsigned long blinkTimeMS = 0;
    const unsigned long blinkDuration = 1000L;  // blink the cursor
};

extern Clock clock;
