#include <Arduino.h>
#include "HSV.h"

word ByteHSVtoRGB(byte h, byte s, byte v)
{
  // save program space
  return ::HSVtoRGB((word)(h << 8), (word)(s << 8), (word)(v << 8));
}

word RGBto565(byte r, byte g, byte b)
{
  // 565 rgb
  return ((b & 0xF8) >> 3) |
         ((g & 0xFC) << 3) |
         ((r & 0xF8) << 8);
}         

// convert Hue, Saturation & Value to Red, Green & Blue
// based on web.mit.edu/storborg/Public/hsvtorgb.c, refactored
// * 16-bit integers rather than 8-bit
// * DON'T calculate temp vars (p, q, t), only those needed for the RGB
// * no grayscale, S != 0
// * combine scaling back with building 565
word HSVtoRGB(word h, word s, word v)
{
  word region;
  unsigned long r, g, b, fpart;

  // one of 6 hue regions
  region = h / 10923L;
  // remainder within region, scaled 0-65535
  fpart = (h - (region * 10923L)) * 6L;

  switch (region)
  {
    case 0:
      r = v;
      g = (unsigned long)(v * (65535L - ((s * (65535L - fpart)) >> 16))) >> 16; // t
      b = (unsigned long)(v * (65535L - s)) >> 16; // p
      break;
    case 1:
      r = (unsigned long)(v * (65535L - ((s * fpart) >> 16))) >> 16; // q
      g = v;
      b = (unsigned long)(v * (65535L - s)) >> 16; // p
      break;
    case 2:
      r = (unsigned long)(v * (65535L - s)) >> 16; // p
      g = v;
      b = (unsigned long)(v * (65535L - ((s * (65535L - fpart)) >> 16))) >> 16;  // t
      break;
    case 3:
      r = (unsigned long)(v * (65535L - s)) >> 16; // p
      g = (unsigned long)(v * (65535L - ((s * fpart) >> 16))) >> 16; // q
      b = v;
      break;
    case 4:
      r = (unsigned long)(v * (65535L - ((s * (65535L - fpart)) >> 16))) >> 16;  // t
      g = (unsigned long)(v * (65535L - s)) >> 16; // p
      b = v;
      break;
    case 5:
      r = v;
      g = (unsigned long)(v * (65535L - s)) >> 16; // p
      b = (unsigned long)(v * (65535L - ((s * fpart) >> 16))) >> 16; // q
      break;
    default:
      return 0;
  }

  // for simplicity, just truncate
  return ( b >> 11) |
         ((g & 0xFC00) >> 5)  |
         ( r & 0xF800);
}
