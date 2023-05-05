#pragma once

#define HUE_Red       0
#define HUE_Brown    21
#define HUE_Yellow   43
#define HUE_Green    85
#define HUE_Cyan    128
#define HUE_Blue    171
#define HUE_Magenta 213

#define SAT_Normal     255
#define SAT_Highlight  192

#define VAL_Normal  255
#define VAL_Shadow   64
#define VAL_Brown    80

word ByteHSVtoRGB(byte h, byte s, byte v);
word RGBto565(byte r, byte g, byte b);
word HSVtoRGB(word h, word s, word v);


