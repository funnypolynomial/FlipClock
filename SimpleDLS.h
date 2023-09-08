#pragma once

#include <Arduino.h>

// A simple Daylight Saving Time adjustment class
// Caller provides the dates DLS changes.  
// The class adjusts the clock hour to and from display but does not adjust the date
// Intended to be "good enough".
class SimpleDLS
{
public:
  static void SetData(const byte* pData);
  
  // Ranges - Hours:0..23, Days:1..31, Months:1..12, Years:0..99

  // RTC hour -> Display hour.  Use to display RTC time adjusted for DLS
  static int GetDisplayHour24();//int ClockHour24, int ClockDay, int ClockMonth, int ClockYear);   HACK to save program space, use RTC members

  // Display hour -> RTC hour. Use when setting the time
  static int GetClockHour24(int DisplayHour24, int ClockDay, int ClockMonth, int ClockYear);

  // Returns true if DLS is active
  static bool Active(int ClockHour24, int ClockDay, int ClockMonth, int ClockYear, const byte* pData);

  // Example DLS data
  static const byte m_pNewZealandData[] PROGMEM;
  static const byte m_pUnitedStatesData[] PROGMEM;

private:
  static unsigned short OrdinalDay(int day, int month);
  static const byte* m_pData;
};
