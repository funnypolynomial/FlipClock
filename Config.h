#pragma once

//#define ENABLE_BLANK_TENS // blank 10s of hours in 12-hour mode (1pm is " 1" vs a "01")

//#define ENABLE_AUTO_DLS // Automatically adjust for DLS, via a lookup table

//#define ENABLE_BLANKING // Blank display at night. See blankCheckInterval below

class Config
{
  public:
    enum Bins { eNoBin, eYellowBin, eRedBin};
    enum Faces { eFlip, eCubes, eTriangles, ePong, eRandom};
    bool mode24Hour = false;
    byte modeFace  = eFlip;
    
    byte binRefNext = eNoBin;
    byte binRefDaysToNext = 0;
    byte binRefYear = 18;
    byte binRefMonth = 1;
    byte binRefDate = 1;
    
    byte binCalcNext = eNoBin;
    byte binCalcDaysToNext = 0;

#ifdef ENABLE_BLANKING
    // HACK "blanking" the display at night. Settings NOT saved, there's no UI.
    static const byte blankCheckInterval = 15;  // check every 15 minutes, 0 turns blanking OFF
    static const byte blankOnHour = 19; // display will be blank after 7pm
    static const byte blankOffHour = 7; // display will be blank before 7am
#endif
    
    void Load();
    void Save();
    int SaveFlags();

    void Configure();

    void UpdateBin();

  private:
    byte Constrain(byte b, byte min, byte max);
    enum Fields { eYear, eMonth, eDate, eDay, eBin, eDaysToBin, eTimeMode, eHour, eMinute, eAll};
    void DrawField(Fields f, int x, int y, const char* str, int len = -1, word foreground = 0xFFFF);
    void DrawFields();
    
    int  DaysInMonth(int Month, int Year);
    int  DayOfCentury(int Date,  // 1..31
                      int Month, // 1..12
                      int Year);  // 2001..2099 (Leap year test is basic)
    byte OtherBin(byte Bin);
    void ComputeNextBin(int TodayDate, int TodayMonth, int TodayYear);

    byte field;
    bool blinkOn;
    
    byte year;
    byte month;
    byte date;
    byte day;
    byte bin;
    byte daysToBin;
    byte timeMode24;
    byte hour;
    byte minute;
};

extern Config config;
