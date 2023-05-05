#pragma once

;class LargeDigits // large digits for the Time
{
  public:
    static const int Width = 110;
    static const int HalfHeight = 74;
    
    static void drawFrame(int x, int y, int w, int h);
    static void drawDigit(int x, int y, bool top, int digit, bool compress, bool dark, bool partial = false);
};

