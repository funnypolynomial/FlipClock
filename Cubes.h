#pragma once

typedef unsigned long int dword;
class Cubes
{
  public:
    void DrawBackground();
    void DrawDate();
    void DrawTime();
    void CheckUpdate();

  private:
    word GetRandomColour();
    dword GetFontFaces(char ch);
    int DrawSmallCubes(int x, int y, int dX, int dY, const char* str, int len);
    void DrawCube(int x, int y, dword ch, bool half = false, word colour = 0xFFFF);
    void DrawCubelet(int x, int y, bool on);
    int currentDigits[4];
    int currentMinute = -1;
    int currentDate = -1;
    unsigned long checkTimeMS = 0;
    const unsigned long checkTimeDuration = 20000L;  // only check time every 20s, reduce glitches?, time jumps to 01:53?? reprogramming interupts I2C?
    bool blinkOn = false;
    unsigned long blinkTimeMS = 0;
    const unsigned long blinkDuration = 1000L;  // blink the cursor
};

extern Cubes cubes;

