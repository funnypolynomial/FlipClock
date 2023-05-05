#include <Arduino.h>
#include "BTN.h"

// buttons

BTN btn1Set;
BTN btn2Adj;

#define HOLD_TIME_MS 50
#define CLOSED_STATE LOW  // pin pulled LOW when pressed
#define OPEN_STATE   HIGH

void BTN::Init(int Pin)
{
  m_iPin = Pin;
  m_iPrevReading = OPEN_STATE;
  m_iPrevState = CLOSED_STATE;
  m_iTransitionTimeMS = millis();
  pinMode(m_iPin, INPUT_PULLUP);
}

bool BTN::CheckButtonPress()
{
  // debounced button, true if button pressed
  int ThisReading = digitalRead(m_iPin);
  if (ThisReading != m_iPrevReading)
  {
    // state change, reset the timer
    m_iPrevReading = ThisReading;
    m_iTransitionTimeMS = millis();
  }
  else if (ThisReading != m_iPrevState &&
           (millis() - m_iTransitionTimeMS) >= HOLD_TIME_MS)
  {
    // a state other than the last one and held for long enough
    m_iPrevState = ThisReading;
    return (ThisReading == CLOSED_STATE);
  }
  return false;
}

bool BTN::IsDown()
{
  // non-debounced, instantaneous reading
  return digitalRead(m_iPin) == CLOSED_STATE;
}

