#include <Arduino.h>
#include "TriangleClock.h"
#include "RTC.h"
#include "Config.h"
#include "SimpleDLS.h"

TriangleClock::TriangleClock(TriangleMesh& mesh):
  m_Mesh(mesh)
{
}

void TriangleClock::Init()
{
  m_BlinkOn = true;
  m_UpdateTimer = m_BlinkTimer = millis();
  m_DisplayedMinute = -1;
  m_Mesh.Clear();
}

void TriangleClock::Loop()
{
  int Minute = rtc.ReadMinute();
  if (RTC::CheckPeriod(m_UpdateTimer, 15000L) || m_DisplayedMinute != Minute)
  {
    if (m_DisplayedMinute != Minute)
    {
      // time to update the display
      m_Mesh.Clear();
      rtc.ReadTime(true);
      int DisplayHour24 = SimpleDLS::GetDisplayHour24();//rtc.m_Hour24, rtc.m_DayOfMonth, rtc.m_Month, rtc.m_Year);

      char buffer[10];
      buffer[0] = 0;
      m_Mesh.RandomizeMesh(true);
      m_UpdateTimer = millis();
      if (config.mode24Hour)
      {
        AppendNum(buffer, DisplayHour24, '0');
        AppendNum(buffer, rtc.m_Minute, '0');
        m_Mesh.Display(buffer, 7, 0);
      }
      else
      {
        Display12Hour(DisplayHour24, buffer);
      }
      m_DisplayedMinute = Minute;
    }
    
    m_Mesh.RandomizeColour(true);
    m_Mesh.Paint();
    m_BlinkOn = true;
  }  
  else if (RTC::CheckPeriod(m_BlinkTimer, 1000))
  {
      if (!config.mode24Hour)
      {
        m_Mesh.BlinkColon(m_BlinkOn);
      }
      m_BlinkOn = !m_BlinkOn;
  }
}

void TriangleClock::Randomize()
{
  rtc.ReadTime(true);
  long seed = long(word(rtc.m_Hour24+1, rtc.m_DayOfMonth) | word(rtc.m_Minute+1, rtc. m_Month)) << 16;
  randomSeed(seed);
}

size_t TriangleClock::AppendNum(char* Buffer, int Num, char Lead)
{
  // just 0..99
  size_t Len = strlen(Buffer);
  Num = Num % 100;
  if (Num < 10)
  {
    if (Lead)
      Buffer[Len++] = Lead;
  }
  else
  {
    Buffer[Len++] = '0' + (Num / 10);
  }
  Buffer[Len++] = '0' + (Num % 10);
  Buffer[Len] = 0;
  return Len;
}

void TriangleClock::Display12Hour(int DisplayHour24, char* buffer)
{
  int row = 7;
  byte Hr = DisplayHour24;
  if (Hr == 0)
    Hr = 12;
  else if (Hr > 12)
    Hr -= 12;
  
  bool ThinOnes = false; 
  if (Hr >= 10)
  {
    // use a thin '1' to make things fit
    ThinOnes = true;
    m_Mesh.Display("/", row, 0);
  }
  int col = 2;
  if (Hr == 11)
  {
    strcpy(buffer, "/");  // double-thin
    col++;
  }
  else
  {
    AppendNum(buffer, Hr % 10, '\0'); // no padding char
  }
  m_Mesh.Display(buffer, row, col);

  // slip in the : between chars
  col = m_Mesh.Display(":", row, 6);
  col -= 2;

  buffer[0] = 0;
  AppendNum(buffer, rtc.m_Minute, '0');
  if (ThinOnes)
  {
    // convert 1's to san-serif versions
    char* pCh = buffer;
    while (*pCh)
    {
      if (*pCh == '1')
        *pCh = '<';
      pCh++;
    }
  }
  m_Mesh.Display(buffer, row, col);
}


