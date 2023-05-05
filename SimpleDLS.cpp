#include "SimpleDLS.h"
#include "RTC.h"

// For simplicity, rather than this code coping with "the last Sunday in September" etc, the caller needs to 
// provide the dates when DLS changes.
// pData is these bytes:
//   DLS hour eg 2 (=2am)
//   DLS start month, eg 9 (=September)
//   DLS end month, eg 4 (=April)
//   DLS date table first year, eg 14 (=2014)
//   DLS date table last year, eg 24 (=2024)
//
//   DLS start day for first year, eg 28 (=28th September 2014)
//   DLS end day for first year, eg 6 (=6th April 2014)
//   DLS start day for second year
//   DLS end day for second year
//   ...
//   DLS start day for last year
//   DLS end day for last year


// For example, this is the data for New Zealand, 2018-2030
// http://www.dia.govt.nz/Daylight-Saving
// https://www.timeanddate.com/time/change/new-zealand?year=2018
const byte SimpleDLS::m_pNewZealandData[] =
{
  2,      // hour (2am)
  9, 4,   // start and end months; September & April
  23, 30, // first and last years; 2023-2030 (in list which follows)

  // Start day (in September), end day (in April)
  24, 2, // 2023
  29, 7,
  28, 6,
  27, 5,
  26, 4,
  24, 2,
  30, 1,
  29, 7, // 2030
  
};

// USA, http://en.wikipedia.org/wiki/Daylight_saving_time_in_the_United_States
const byte SimpleDLS::m_pUnitedStatesData[] =
{
  2,       // hour (2am)
  3,  11,  // start and end months; March & November
  23, 30,  // first and last years; 2023-2030 (in list which follows)

  12, 5,  // 2023
  10, 3,
  9,  2,
  8,  1,
  14, 7,
  12, 5,
  11, 4,
  10, 3,  // 2030 
};

const byte* SimpleDLS::m_pData = NULL;

void SimpleDLS::SetData(const byte* pData)
{
  m_pData = pData;
}

int SimpleDLS::GetDisplayHour24()//int ClockHour24, int ClockDay, int ClockMonth, int ClockYear) HACK to save space
{
  // Takes the hour from the RTC and adjusts it for display by adding a DLS hour if applicable
  // Assumes the date is correct. 
  // Does not adjust the date. 
  int ClockHour24 = rtc.m_Hour24; int ClockDay = rtc.m_DayOfMonth; int ClockMonth = rtc.m_Month; int ClockYear = rtc.m_Year;
  int DisplayHour24 = ClockHour24;
  if (m_pData && Active(ClockHour24, ClockDay, ClockMonth, ClockYear, m_pData))
  {
    // if active, add an hour
    if (DisplayHour24 >= 23)
    {
      DisplayHour24 = 0;
    }
    else
    {
      DisplayHour24++;
    }
  }
  return DisplayHour24;
}

int SimpleDLS::GetClockHour24(int DisplayHour24, int ClockDay, int ClockMonth, int ClockYear)
{
  // Takes the hour which the RTC should be set to, if the user has selected the DisplayHour
  // Assumes the date is correct (and that you're not setting the time early in the morning on a DLS change day!
  // Does not adjust the date. 

  int ClockHour24 = DisplayHour24;
  if (m_pData && Active(ClockHour24, ClockDay, ClockMonth, ClockYear, m_pData))
  {
    // if active, subtract an hour
    if (ClockHour24 > 0)
    {
      ClockHour24--;
    }
    else
    {
      ClockHour24 = 23;
    }
  }
  return ClockHour24;
}

bool SimpleDLS::Active(int ClockHour24, int ClockDay, int ClockMonth, int ClockYear, const byte* pData)
{
  // returns true if DLS is active
  int DLSHour       = pgm_read_byte(pData++);
  int DLSStartMonth = pgm_read_byte(pData++);
  int DLSEndMonth   = pgm_read_byte(pData++);
  int FirstYear     = pgm_read_byte(pData++);
  int LastYear      = pgm_read_byte(pData++);

  bool Active = false;
  if (FirstYear <= ClockYear && ClockYear <= LastYear)
  {
    // in our table, jump to the current year's entries
    pData += (ClockYear - FirstYear)*2;
    // get ordinal values for today and the start/end days
    unsigned short Today = OrdinalDay(ClockDay,   ClockMonth);
    unsigned short Start = OrdinalDay(pgm_read_byte(pData++), DLSStartMonth);
    unsigned short End   = OrdinalDay(pgm_read_byte(pData),     DLSEndMonth);
    if (Today == Start)
    {
      // start day, starts at the DLS hour
      Active = (ClockHour24 >= DLSHour);
    }
    else if (Today == End)
    {
      // end day, still active before the DLS hour
      Active = (ClockHour24 < DLSHour);
    }
    else if (Start < End)
    {
      // within the start/end
      Active = (Start <= Today && Today < End);
    }
    else
    {
      // within the start/end (but they span New Year's Day)
      Active = (Start <= Today || Today < End);
    }
  }
  return Active;
}

unsigned short SimpleDLS::OrdinalDay(int day, int month)
{
  // Convert day and month to an ORDINAL VALUE: month*32 + day
  // This is not the same as http://en.wikipedia.org/wiki/Ordinal_date but lets us compare dates
  return (unsigned short)(((month) << 5) | day);
}
