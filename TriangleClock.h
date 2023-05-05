#pragma once
#include "TriangleMesh.h"

class TriangleClock
{
  public:
    TriangleClock(TriangleMesh& mesh);
    
    void Init();
    void Loop();
    
    static size_t AppendNum(char* Buffer, int Num, char Lead);

  private:
    void Randomize();
    void Display12Hour(int DisplayHour24, char* buffer);
    
    TriangleMesh& m_Mesh;
    unsigned long m_UpdateTimer;
    unsigned long m_BlinkTimer;
    bool m_BlinkOn;
    int m_DisplayedMinute;
};


