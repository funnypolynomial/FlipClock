#pragma once

// Talk to the DS3231

class RTC
{
  public:
    RTC();
    void Setup();
    byte BCD2Dec(byte BCD);
    byte Dec2BCD(byte Dec);
    void ReadTime(bool Full=false);
    byte ReadSecond(void);
    byte ReadMinute(void);
    void WriteTime(void);

    byte m_Hour24;      // 0..23
    byte m_Minute;      // 0..59
    byte m_Second;      // 0..59
    byte m_DayOfWeek;   // 1..7 
    byte m_DayOfMonth;  // 1..31
    byte m_Month;       // 1..12
    byte m_Year;        // 0..99

    static bool CheckPeriod(unsigned long& Timer, unsigned long PeriodMS);
};

extern RTC rtc;

