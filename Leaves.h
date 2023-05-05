#pragma once
// some common declarations

// see http://www.barth-dev.de/online/rgb565-color-picker/
const word foregroundColour = 0xFFFF; // 0xCE79
const word darkForegroundColour = 0xBDD7;
const word backgroundColour = 0x528A;
const word darkBackgroundColour = 0x31A6;
const word yellowBin = 0xFFE4;
const word redBin = 0xF904;

// letter/digit RLE encoding:
#define BACK(_b) (byte)(_b)
#define FORE(_f) (byte)(_f)
#define END      (byte)(0x80)


