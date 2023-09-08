#include <Arduino.h>
#include "SoftwareI2C.h"
#include "Pins.h"
#include "RTC.h"

// real time clock
RTC rtc;

// we can't use Wire because ‎A4 (SDA) resets the LCD
SoftwareI2C softWire;

#define RTC_I2C_ADDRESS 0x68

RTC::RTC():
    m_Hour24(0),
    m_Minute(0),
    m_Second(0),
    m_DayOfWeek(1), 
    m_DayOfMonth(1),
    m_Month(1),
    m_Year(12)
{
}

void RTC::Setup(void)
{
  softWire.begin(PIN_SDA, PIN_SCL);
  ReadTime(true);
}

byte RTC::BCD2Dec(byte BCD)
{
  return (BCD/16*10) + (BCD & 0x0F);
}

byte RTC::Dec2BCD(byte Dec)
{
  return (Dec/10*16) + (Dec % 10);
}

void RTC::ReadTime(bool Full)
{
  if (Full)
  {
    // from register 0
    softWire.beginTransmission(RTC_I2C_ADDRESS);
    softWire.write((byte)0x00);
    softWire.endTransmission();
   
    softWire.requestFrom(RTC_I2C_ADDRESS, 7);
   
    m_Second = BCD2Dec(softWire.read() & 0x7F);  // high bit is CH (Clock Halt)
  }
  else
  {
    // just the minutes and hours
    // from register 1
    softWire.beginTransmission(RTC_I2C_ADDRESS);
    softWire.write((byte)0x01);
    softWire.endTransmission();
   
    softWire.requestFrom(RTC_I2C_ADDRESS, 2);
  }
  m_Minute = BCD2Dec(softWire.read());
  byte Register2 = softWire.read();
  if (Register2 & 0x40)  // 12/24 hr
  {
    // 12 hr mode, bit 6=PM
    m_Hour24 = BCD2Dec(Register2 & 0x3F);
    if (Register2 & 0x20)
    {
      m_Hour24 += 12;
      if (m_Hour24 > 23) m_Hour24 = 0;
    }
  }
  else
  {
    // 24 hour mode
    m_Hour24 = BCD2Dec(Register2 & 0x3F);
  }
  
  if (Full)
  {
    m_DayOfWeek  = BCD2Dec(softWire.read());
    m_DayOfMonth = BCD2Dec(softWire.read());
    m_Month      = BCD2Dec(softWire.read());
    m_Year       = BCD2Dec(softWire.read());
  }
}

byte RTC::ReadSecond(void)
{
  // from register 01
  softWire.beginTransmission(RTC_I2C_ADDRESS);
  softWire.write(0x00);
  softWire.endTransmission();
 
  softWire.requestFrom(RTC_I2C_ADDRESS, 1);
 
  m_Second = BCD2Dec(softWire.read());
  return m_Second;
}

byte RTC::ReadMinute(void)
{
  // from register 01
  softWire.beginTransmission(RTC_I2C_ADDRESS);
  softWire.write(0x01);
  softWire.endTransmission();
 
  softWire.requestFrom(RTC_I2C_ADDRESS, 1);
 
  return BCD2Dec(softWire.read());
}

void RTC::WriteTime(void)
{
   softWire.beginTransmission(RTC_I2C_ADDRESS);
   softWire.write((byte)0x00);
   softWire.write(Dec2BCD(m_Second));
   softWire.write(Dec2BCD(m_Minute));
   softWire.write(Dec2BCD(m_Hour24));  // 24 hr mode
   softWire.write(Dec2BCD(m_DayOfWeek));
   softWire.write(Dec2BCD(m_DayOfMonth));
   softWire.write(Dec2BCD(m_Month));
   softWire.write(Dec2BCD(m_Year));
   softWire.endTransmission();
}

bool RTC::CheckPeriod(unsigned long& Timer, unsigned long PeriodMS)
{
  // has the timer interval gone off?
  unsigned long Now = millis();
  if (Now - Timer > PeriodMS)
  {
    Timer = Now;
    return true;
  }
  return false;
}
