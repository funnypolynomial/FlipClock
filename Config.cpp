#include "arduino.h"
// disable warnings in EEPROM.h
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include "EEPROM.h"
#include "ILI948x.h"
#include "Leaves.h"
#include "LargeDigits.h"
#include "SmallChars.h"
#include "Pins.h"
#include "BTN.h"
#include "RTC.h"
#include "SimpleDLS.h"
#include "Clock.h"
#include "Config.h"

#define MIN_YEAR 18
#define MAX_YEAR 38
Config config;

// EEPROM contains
// 0: flags, 0b0000MMMH H is 1 if 24-hour mode, MMM is face Flip/Cubes/Triangles/Pong/Random
// the rest are the red bin/yellow bin reference info
// 1: bin
// 2: days to bin
// 3: year
// 4: month
// 5: date
void Config::Load()
{
  int address = 0;
  byte value = EEPROM.read(address++);
  if (value & 0x80) // assume uninitialised
  {
    value = 0x00;
  }
  mode24Hour = (value & 0x01);
  modeFace  = (value & 0x0E) >> 1;
  value = EEPROM.read(address++);
  if (value <= eRedBin)
  {
    binRefNext = value;
    binRefDaysToNext = Constrain(EEPROM.read(address++), 0, 7);
    binRefYear = Constrain(EEPROM.read(address++), MIN_YEAR, MAX_YEAR);
    binRefMonth  = Constrain(EEPROM.read(address++), 1, 12);
    binRefDate  = Constrain(EEPROM.read(address++), 1, DaysInMonth(binRefMonth, 2000 + binRefYear));
    UpdateBin();
  }
  else  // assume uninitialised
  {
    binRefNext = eNoBin;
    binRefDaysToNext = 0;
  }
}

void Config::Save()
{
  int address = SaveFlags();
  EEPROM.write(address++, binRefNext);
  EEPROM.write(address++, binRefDaysToNext);
  EEPROM.write(address++, binRefYear);
  EEPROM.write(address++, binRefMonth);
  EEPROM.write(address++, binRefDate);
}

int Config::SaveFlags()
{
  int address = 0;
  byte flags = mode24Hour?0x01:0x00;
  flags |= modeFace << 1;
  EEPROM.write(address++, flags);
  return address;
}

const char binNames[] = "NO BIN"
                        "YLO IN"
                        "RED IN";
void Config::Configure()
{
  bool save = true;
  ILI948x::ColourWord(backgroundColour, ILI948x::Window(0, 0, LCD_WIDTH, LCD_HEIGHT));
  rtc.ReadTime(true);
  year = rtc.m_Year;
  month = rtc.m_Month;
  date = rtc.m_DayOfMonth;
  day =  rtc.m_DayOfWeek;
  hour = SimpleDLS::GetDisplayHour24();//rtc.m_Hour24, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year);
  minute = rtc.m_Minute;
  bin = binCalcNext;
  daysToBin = binCalcDaysToNext;
  timeMode24 = mode24Hour?1:0;
  field = eAll;
  unsigned long blinkTimeMS = millis();
  unsigned long idleTimeMS = blinkTimeMS;
  blinkOn = true;

  bool update = true;
  while (true)
  {
    if (update)
    {
      DrawFields();
      update = false;
      if (field == eAll)
      {
        field = eYear;
      }
    }
    
    if (btn2Adj.CheckButtonPress()) // next value
    {
      switch (field)
      {
        case eYear:
          year = (year > MAX_YEAR)?MIN_YEAR:year + 1;
          break;
        case eMonth:
          month = (month == 12)?1:month + 1;
          break;
        case eDate:
          date = (date >= DaysInMonth(month, 2000 + year))?1:date + 1;
          break;
        case eDay:
          day = (day == 7)?1:day + 1;
          break;
        case eBin:
          bin = (bin == eRedBin)?eNoBin:bin + 1;
          break;
        case eDaysToBin:
          daysToBin = (daysToBin == 7)?0:daysToBin + 1;
          break;
        case eTimeMode:
          timeMode24 = (timeMode24 == 1)?0:1;
          break;
        case eHour:
          hour = (hour == 23)?0:hour + 1;
          break;
        case eMinute:
          minute = (minute == 59)?0:minute + 1;
          break;
        default:
          break;
      }
      idleTimeMS = blinkTimeMS = millis();
      blinkOn = true;
      update = true;
    }
    else if (btn1Set.CheckButtonPress())  // next field
    {
      idleTimeMS = blinkTimeMS = millis();
      blinkOn = true;
      DrawFields();
      field++;
      if (field == eDate)
        date = Constrain(date, 1, DaysInMonth(month, 2000 + year));
      if (field == eAll)
      {
        save = true;
        break;
      }
    }
    else
    {
      unsigned long nowMS = millis();
      if ((nowMS - blinkTimeMS) > 500L) // half second blink
      {
        blinkTimeMS = millis();
        blinkOn = !blinkOn;
        update = true;
      }
      if ((nowMS - idleTimeMS) > 30000L)  // idle 30s, bail out
      {
        save = false;
        break;
      }
    }
  }

  if (save)
  {
    rtc.m_Year =         year;
    rtc.m_Month =        month; 
    rtc.m_DayOfMonth =   date;  
    rtc.m_DayOfWeek =    day;   
    rtc.m_Hour24 =       SimpleDLS::GetClockHour24(hour, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year);
    rtc.m_Minute =       minute;
    rtc.m_Second =       0;
    rtc.WriteTime();
    binRefNext = bin;
    binRefDaysToNext = daysToBin;
    binRefYear = year;
    binRefMonth = month;
    binRefDate = date;
    mode24Hour = timeMode24;
    Save();
    UpdateBin();
  }
}

void Config::UpdateBin()
{
  rtc.ReadTime(true);
  ComputeNextBin(rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year);
}

void Config::DrawFields()
{
  int x1 = 5;
  int x2 = LCD_WIDTH/2;
  int x3 = x2 + SmallChars::Width*2 + 40;
  int y0 = 10;
  int dY = SmallChars::HalfHeight*2 + 20;
  int y = y0;
  char buffer[8];
  buffer[0] = '2';
  buffer[1] = '0';
  clock.GetNumStr(year, buffer+2, false);
  DrawField(eYear, x1, y, buffer); 
  DrawField(eMonth, x2, y, clock.GetMonthName(month), 3); 
  y += dY;
  DrawField(eDate, x1, y, clock.GetNumStr(date, buffer, true)); 
  DrawField(eDay, x2, y, clock.GetDayName(day), 3); 
  y += dY;
  if (bin == eNoBin)
    DrawField(eBin, x1, y, binNames + bin*6, 6);
  else
    DrawField(eBin, x1, y, binNames + bin*6, 6, bin == eYellowBin?yellowBin:redBin);
  buffer[0] = '0'+ daysToBin;
  buffer[1] = 'D';
  DrawField(eDaysToBin, x3, y, buffer, 2); 
  y += dY;
  DrawField(eTimeMode, x1, y, timeMode24?"24H":"12H"); 
  DrawField(eHour, x2, y, clock.GetNumStr(hour, buffer, false));  
  DrawField(eMinute, x3, y, clock.GetNumStr(minute, buffer, false));  
}

void Config::DrawField(Fields f, int x, int y, const char* str, int len, word foreground)
{
  if (field == f)
  {
    SmallChars::drawStr(x, y, str, len, blinkOn?foreground:backgroundColour, false); 
  }
  else if (field == eAll)
  {
    SmallChars::drawStr(x, y, str, len, foreground, true); 
  }
}


byte Config::Constrain(byte b, byte min, byte max)
{
  if (b < min) return min;
  else if (b > max) return max;
  else return b;
}

// <Days-in-Month>-28 encoded as 2bits/Month 0b0000DD..FFJJ00
#define MONTH_LENGTHS (0x03BBEECC)

int Config::DaysInMonth(int Month, int Year)  // 1..12, 2001..2099
{
  int Increment = (MONTH_LENGTHS >> (Month*2) & 0x03);
  if (Year % 4 || Increment)
  {
    return 28 + Increment;
  }
  else
  {
    // Leap year AND Feb
    return 29;
  }
}

int Config::DayOfCentury(int Date,  // 1..31
                         int Month, // 1..12
                         int Year)  // 2001..2099 (Leap year test is basic)
{
  // years into the "century"
  Year -= 2000;
  // days in whole years
  int Day = Date + 365*Year;
  // leap days (except in 2000)
  Day += (Year - 1)/4;

  // days in this year
  while (--Month > 0) 
    Day += DaysInMonth(Month, Year);
  return Day;
}

byte Config::OtherBin(byte Bin)
{
  // red<->yellow
  if (Bin != eNoBin)
    Bin = (Bin == eRedBin)?eYellowBin:eRedBin;
  return Bin;
}

void Config::ComputeNextBin(int TodayDate, int TodayMonth, int TodayYear)
{
  // given the reference date, bin and offset, and today's date, compute the NEXT bin and days until it is due
  binCalcNext = eNoBin;
  binCalcDaysToNext = 0;
  if (binRefNext == eNoBin)
    return;

  int referenceDayOfCentury = DayOfCentury(binRefDate, binRefMonth, 2000 + binRefYear);
  int todayDayOfCentury = DayOfCentury(TodayDate, TodayMonth, 2000 + TodayYear);
  // adjust reference back by a whole cycle (a fortnight) then forward by the offset
  referenceDayOfCentury -= 14;
  referenceDayOfCentury += binRefDaysToNext;

  int daysBetween = todayDayOfCentury - referenceDayOfCentury;
  if (daysBetween > 0)  // only the future
  {
    int weeksBetween  = daysBetween / 7;
    int daysRemainder = daysBetween % 7;
    if (weeksBetween % 2)
    {
      // ODD number of whole weeks between; reference bin is next
      binCalcNext = binRefNext;
    }
    else
    {
      // EVEN number of weeks between; other bin is next
      binCalcNext = OtherBin(binRefNext);
    }

    if (daysRemainder == 0)
    {
      // EXACT multiple, invert logic above
      binCalcNext = OtherBin(binCalcNext);
    }

    binCalcDaysToNext = (7 - daysRemainder) % 7;
  }
}
