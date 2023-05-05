#pragma once

class BTN
{
  public:
    void Init(int Pin);
    bool CheckButtonPress();
    bool IsDown();
    
  private:
    int m_iPin;
    int m_iPrevReading;
    int m_iPrevState;
    unsigned long m_iTransitionTimeMS;
};

extern BTN btn1Set;
extern BTN btn2Adj;

