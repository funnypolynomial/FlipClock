#pragma once

class SmallChars  // smaller letters and numbers for the Date
{
  public: 
    static const int Width = 46;
    static const int HalfHeight = 30;
    
    static void drawFrame(int x, int y, int w, int h);
    static void drawChar(int x, int y, bool top, char ch, word foreground, bool reduce = false, bool narrow = false, word background = 0);
    static void drawStr(int x, int y, const char* str, int len = -1, word foreground = 0xFFFF, bool background = true);
};
