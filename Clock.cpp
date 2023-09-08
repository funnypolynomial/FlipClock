#include <Arduino.h>
#include "ILI948x.h"
#include "Leaves.h"
#include "LargeDigits.h"
#include "SmallChars.h"
#include "Pins.h"
#include "BTN.h"
#include "RTC.h"
#include "Config.h"
#include "SimpleDLS.h"
#include "Clock.h"

//#define DEMO  // just show the leaves falling

// the flip clock main logic
Clock clock;

// terminology: "split flap" "leaves" "leaf"


#define SECONDS_WIDTH (SmallChars::Width/2)
#define SECONDS_HEIGHT (4*SmallChars::HalfHeight/3)
#define SECONDS_X ((LCD_WIDTH - SECONDS_WIDTH)/2)
#define SECONDS_Y (BigDigitY() - SECONDS_HEIGHT - 20)

void Clock::DrawBackground()
{
  // draw the static bits
  ILI948x::ColourWord(backgroundColour, ILI948x::Window(0, 0, LCD_WIDTH, LCD_HEIGHT));
  for (int digit = 0; digit <= 3; digit++)
  {
    LargeDigits::drawFrame(digitOriginX + digit*interDigitX, BigDigitY(), LargeDigits::Width, LargeDigits::HalfHeight*2 + hemiDigitYGap);
  }
  // for the seconds colon (2/3rds small char)
  SmallChars::drawFrame(SECONDS_X, SECONDS_Y, SECONDS_WIDTH, SECONDS_HEIGHT + 4);
}

void Clock::DrawTime()
{
  // just the time
#ifdef DEMO
  ILI948x::DisplayOn();
  for (int digit = 0; digit <= 3; digit++)
    BigDigit(digitOriginX + digit*interDigitX, BigDigitY(), 1, 0);
  for (int digit = 0; digit <= 3; digit++)
    for (int step = 0; step <= digit; step++)
      BigDigit(digitOriginX + digit*interDigitX, BigDigitY(), 1, step, 2);
  SERIALISE_COMMENT("*** DEMO");
  delay(10000L);
#else
  getTimeDigits(currentDigits);
  currentMinute = rtc.m_Minute;
  for (int digit = 0; digit <= 3; digit++)
  {
    BigDigit(digitOriginX + digit*interDigitX, BigDigitY(), currentDigits[digit], 0);
  }
#endif  
}

void Clock::DrawDate()
{
  // just the date
  updateDate();
  currentDate = rtc.m_DayOfMonth;
}

void Clock::CheckUpdate()
{
  // time to update display?
  if (RTC::CheckPeriod(checkTimeMS, checkTimeDuration))
  {
    int thisMinute = rtc.ReadMinute();
    if (thisMinute != currentMinute)
    {
      updateTime();
      currentMinute = thisMinute;
      if (currentDate != rtc.m_DayOfMonth)
      {
        updateDate();
        currentDate = rtc.m_DayOfMonth;
      }
    }
  }
  
  if (RTC::CheckPeriod(blinkTimeMS, blinkDuration))
  {
    // the flipping/blinking colon
    char prevCh = blinkOn?':':' ';
    blinkOn = !blinkOn;
    char ch = blinkOn?':':' ';
    // simple animation
    unsigned long start = millis();
    SmallChars::drawChar(SECONDS_X, SECONDS_Y, true, ch, foregroundColour, true, true);
    SmallChars::drawChar(SECONDS_X, SECONDS_Y + SECONDS_HEIGHT/2 + 2, false, prevCh, darkForegroundColour, true, true, 0x4A49);//darkBackgroundColour);
    while ((millis() - start) < 75L) 
      ;
    SmallChars::drawChar(SECONDS_X, SECONDS_Y + SECONDS_HEIGHT/2 + 2, false, ch, foregroundColour, true, true);
    SERIALISE_COMMENT(blinkOn?"*** On":"");
  }
}

void Clock::BigDigit(int x, int y, int digit1, int step, int digit2)
{
  // draw digit1 at x,y at the given step of flipping to the next digit2
  // a digit of 10 is interpreted as a blank
  if (step == 0)  // normal
  {
    LargeDigits::drawDigit(x, y, true, digit1, false, false);                  // top of 1                 |
    LargeDigits::drawDigit(x, y + lowerYOffset, false, digit1, false, false);  // bottom of 1              |
  }
  else if (step == 1) // top flipped down upper 41 degrees (=acos(3/4), close enough to 45!)
  {
    LargeDigits::drawDigit(x, y, true, digit2, false, false,    true);         // peek at top of 2         |/
    LargeDigits::drawDigit(x, y, true, digit1, true, true);                    // top of 1, tipped, shadow |
                                                                               // bottom of 1 remains
  }
  else if (step == 2) // top flipped 90 deg
  {
    LargeDigits::drawDigit(x, y, true, digit2, false, false);                  // top of 2                 |_
    LargeDigits::drawDigit(x, y + lowerYOffset, false, digit1, false, true);   // bottom of 1, shadow      |
  }
  else if (step == 3) // top flipped down lower 131 degrees (close enough to 135!)
  {
                                                                               // top of 2 remains         |
    LargeDigits::drawDigit(x, y + lowerYOffset, false, digit2, true, false);   /* bottom of 2, tipped      |\
                                                                               */
  }
}

// fills in digits array from the current time (12-hour mode), digits not chars
void Clock::getTimeDigits(int* digits)
{
  rtc.ReadTime(true);
  int hour = SimpleDLS::GetDisplayHour24();
  if (!config.mode24Hour)
  {
    if (hour > 12)
      hour -= 12;
    if (hour == 0)
      hour = 12;
  }
  digits[0] = hour / 10;
  digits[1] = hour % 10;
  digits[2] = rtc.m_Minute / 10;
  digits[3] = rtc.m_Minute % 10;
#ifdef ENABLE_BLANK_TENS      
  if (!config.mode24Hour && hour < 10)
    digits[0] = 10; // blank
#endif
}

void Clock::updateTime()
{
  int newDigits[4];
  rtc.ReadTime(true);
  getTimeDigits(newDigits);

  for (int digit = 3; digit >= 0; digit--)
  {
    if (newDigits[digit] != currentDigits[digit])
    {
      // animate flip
      for (int step = 1; step < 4; step++)
      {
        unsigned long start = millis();
        BigDigit(digitOriginX + digit*interDigitX, BigDigitY(), currentDigits[digit], step, newDigits[digit]);
        while (millis() - start < 75L)  // normalize the animation frame delay
          ;
      }
      // draw new digit
      currentDigits[digit] = newDigits[digit];
      BigDigit(digitOriginX + digit*interDigitX, BigDigitY(), currentDigits[digit], 0);
    }
  }
}

const char daysOfWeek[]  = "MONTUEWEDTHUFRISATSUN";
const char monthsOfYear[]  = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";

void Clock::updateDate()
{
  char date[3];
  GetNumStr(rtc.m_DayOfMonth, date, true);
  int y = 10;
  SmallChars::drawStr( 10, y, GetDayName(rtc.m_DayOfWeek), 3, 0xFFFF);
  SmallChars::drawStr(190, y, date, 2, 0xFFFF);
  SmallChars::drawStr(320, y, GetMonthName(rtc.m_Month), 3, 0xFFFF);

  config.UpdateBin();
  if (config.binCalcNext != Config::eNoBin)
  {
    // squeeze in a two-thirds-height bin indicator on the right
    int x = LCD_WIDTH - SmallChars::Width - 8;
    y +=  SmallChars::HalfHeight*2 + 20;
    char daysCh = '0' + config.binCalcDaysToNext;
    word colour = (config.binCalcNext == Config::eYellowBin)?yellowBin:redBin;
    SmallChars::drawFrame(x, y, SmallChars::Width, 4*SmallChars::HalfHeight/3 + 4);
    SmallChars::drawChar(x, y, true, daysCh, colour, true);
    SmallChars::drawChar(x, y + 2*SmallChars::HalfHeight/3 + 2, false, daysCh, colour, true);
  }
}

const char* Clock::GetMonthName(byte Month)
{
  return monthsOfYear + 3*(Month - 1);
}

const char* Clock::GetDayName(byte Day)
{
  return daysOfWeek + 3*(Day - 1);
}

char* Clock::GetNumStr(byte Num, char* Buffer, bool noLeadingZero)
{
  Buffer[0] = '0' + Num / 10; 
  Buffer[1] = '0' + Num % 10;
  Buffer[2] = 0;
  if (Buffer[0] == '0' && noLeadingZero)
    Buffer[0] = ' ';
  return Buffer;
}

int Clock::BigDigitY()
{
  if (config.binCalcNext != Config::eNoBin)
    return midY + 10; // make room for bindicator
  else
    return midY;  
}
