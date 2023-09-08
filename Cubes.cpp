#include <Arduino.h>
#include "ILI948x.h"
#include "RTC.h"
#include "Leaves.h"
#include "SimpleDLS.h"
#include "Config.h"
#include "HSV.h"
#include "LargeDigits.h"
#include "Clock.h"
#include "Cubes.h"

// isometric cube font, https://www.dafont.com/kubics-rube.font

Cubes cubes;

#undef END
// packs face number plus hz pixel span plus flags in a WORD:
#define SPAN(_Face,_Span) (word)((_Face << 8) | (_Span & 255))
// word is 0bEV0FFFFFSSSSSSSS (E=row end, V=vert start, F=face, S=span)
#define VERT        (word)(0x4000)
#define END         (word)(0x8000)
#define CubeWidth  18
#define CubeHeight 18
#define CubeStepUp 9
// RLE scanlines for faces in 3x3x3 cubes
const word PROGMEM faceSpans[] = 
{
  SPAN(30, 52), SPAN(23,  4)+END,
  SPAN(30, 50), SPAN(23,  8)+END,
  SPAN(30, 48), SPAN(23, 12)+END,
  SPAN(30, 46), SPAN(23, 16)+END,
  SPAN(30, 44), SPAN(23, 20)+END,
  SPAN(30, 42), SPAN(23, 24)+END,
  SPAN(30, 40), SPAN(23, 28)+END,
  SPAN(30, 38), SPAN(23, 32)+END,
  SPAN(30, 36), SPAN(23, 36)+END,
  SPAN(30, 34), SPAN(22,  4), SPAN(23, 32), SPAN(26,  4)+END,
  SPAN(30, 32), SPAN(22,  8), SPAN(23, 28), SPAN(26,  8)+END,
  SPAN(30, 30), SPAN(22, 12), SPAN(23, 24), SPAN(26, 12)+END,
  SPAN(30, 28), SPAN(22, 16), SPAN(23, 20), SPAN(26, 16)+END,
  SPAN(30, 26), SPAN(22, 20), SPAN(23, 16), SPAN(26, 20)+END,
  SPAN(30, 24), SPAN(22, 24), SPAN(23, 12), SPAN(26, 24)+END,
  SPAN(30, 22), SPAN(22, 28), SPAN(23,  8), SPAN(26, 28)+END,
  SPAN(30, 20), SPAN(22, 32), SPAN(23,  4), SPAN(26, 32)+END,
  SPAN(30, 18), SPAN(22, 36), SPAN(26, 36)+END,
  SPAN(30, 16), SPAN(21,  4), SPAN(22, 32), SPAN(25,  4), SPAN(26, 32), SPAN(29,  4)+END,
  SPAN(30, 14), SPAN(21,  8), SPAN(22, 28), SPAN(25,  8), SPAN(26, 28), SPAN(29,  8)+END,
  SPAN(30, 12), SPAN(21, 12), SPAN(22, 24), SPAN(25, 12), SPAN(26, 24), SPAN(29, 12)+END,
  SPAN(30, 10), SPAN(21, 16), SPAN(22, 20), SPAN(25, 16), SPAN(26, 20), SPAN(29, 16)+END,
  SPAN(30,  8), SPAN(21, 20), SPAN(22, 16), SPAN(25, 20), SPAN(26, 16), SPAN(29, 20)+END,
  SPAN(30,  6), SPAN(21, 24), SPAN(22, 12), SPAN(25, 24), SPAN(26, 12), SPAN(29, 24)+END,
  SPAN(30,  4), SPAN(21, 28), SPAN(22,  8), SPAN(25, 28), SPAN(26,  8), SPAN(29, 28)+END,
  SPAN(30,  2), SPAN(21, 32), SPAN(22,  4), SPAN(25, 32), SPAN(26,  4), SPAN(29, 32)+END,
  SPAN(21, 36), SPAN(25, 36), SPAN(29, 36)+END,
  SPAN( 1,  2)+VERT, SPAN(21, 32), SPAN(24,  4), SPAN(25, 32), SPAN(28,  4), SPAN(29, 32), SPAN(13,  2)+END,
  SPAN( 1,  4)+VERT, SPAN(21, 28), SPAN(24,  8), SPAN(25, 28), SPAN(28,  8), SPAN(29, 28), SPAN(13,  4)+END,
  SPAN( 1,  6)+VERT, SPAN(21, 24), SPAN(24, 12), SPAN(25, 24), SPAN(28, 12), SPAN(29, 24), SPAN(13,  6)+END,
  SPAN( 1,  8)+VERT, SPAN(21, 20), SPAN(24, 16), SPAN(25, 20), SPAN(28, 16), SPAN(29, 20), SPAN(13,  8)+END,
  SPAN( 1, 10)+VERT, SPAN(21, 16), SPAN(24, 20), SPAN(25, 16), SPAN(28, 20), SPAN(29, 16), SPAN(13, 10)+END,
  SPAN( 1, 12)+VERT, SPAN(21, 12), SPAN(24, 24), SPAN(25, 12), SPAN(28, 24), SPAN(29, 12), SPAN(13, 12)+END,
  SPAN( 1, 14)+VERT, SPAN(21,  8), SPAN(24, 28), SPAN(25,  8), SPAN(28, 28), SPAN(29,  8), SPAN(13, 14)+END,
  SPAN( 1, 16)+VERT, SPAN(21,  4), SPAN(24, 32), SPAN(25,  4), SPAN(28, 32), SPAN(29,  4), SPAN(13, 16)+END,
  SPAN( 1, 18)+VERT, SPAN(24, 36), SPAN(28, 36), SPAN(13, 18)+END,
  SPAN( 1, 18)+VERT, SPAN( 2,  2)+VERT, SPAN(24, 32), SPAN(27,  4), SPAN(28, 32), SPAN(12,  2), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2,  4)+VERT, SPAN(24, 28), SPAN(27,  8), SPAN(28, 28), SPAN(12,  4), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2,  6)+VERT, SPAN(24, 24), SPAN(27, 12), SPAN(28, 24), SPAN(12,  6), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2,  8)+VERT, SPAN(24, 20), SPAN(27, 16), SPAN(28, 20), SPAN(12,  8), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2, 10)+VERT, SPAN(24, 16), SPAN(27, 20), SPAN(28, 16), SPAN(12, 10), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2, 12)+VERT, SPAN(24, 12), SPAN(27, 24), SPAN(28, 12), SPAN(12, 12), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2, 14)+VERT, SPAN(24,  8), SPAN(27, 28), SPAN(28,  8), SPAN(12, 14), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2, 16)+VERT, SPAN(24,  4), SPAN(27, 32), SPAN(28,  4), SPAN(12, 16), SPAN(13, 18)+VERT+END,
  SPAN( 1, 18)+VERT, SPAN( 2, 18)+VERT, SPAN(27, 36), SPAN(12, 18), SPAN(13, 18)+VERT+END,
  SPAN( 4,  2)+VERT, SPAN( 1, 16), SPAN( 2, 18)+VERT, SPAN( 3,  2)+VERT, SPAN(27, 32), SPAN(11,  2), SPAN(12, 18)+VERT, SPAN(13, 16)+VERT, SPAN(16,  2)+END,
  SPAN( 4,  4)+VERT, SPAN( 1, 14), SPAN( 2, 18)+VERT, SPAN( 3,  4)+VERT, SPAN(27, 28), SPAN(11,  4), SPAN(12, 18)+VERT, SPAN(13, 14)+VERT, SPAN(16,  4)+END,
  SPAN( 4,  6)+VERT, SPAN( 1, 12), SPAN( 2, 18)+VERT, SPAN( 3,  6)+VERT, SPAN(27, 24), SPAN(11,  6), SPAN(12, 18)+VERT, SPAN(13, 12)+VERT, SPAN(16,  6)+END,
  SPAN( 4,  8)+VERT, SPAN( 1, 10), SPAN( 2, 18)+VERT, SPAN( 3,  8)+VERT, SPAN(27, 20), SPAN(11,  8), SPAN(12, 18)+VERT, SPAN(13, 10)+VERT, SPAN(16,  8)+END,
  SPAN( 4, 10)+VERT, SPAN( 1,  8), SPAN( 2, 18)+VERT, SPAN( 3, 10)+VERT, SPAN(27, 16), SPAN(11, 10), SPAN(12, 18)+VERT, SPAN(13,  8)+VERT, SPAN(16, 10)+END,
  SPAN( 4, 12)+VERT, SPAN( 1,  6), SPAN( 2, 18)+VERT, SPAN( 3, 12)+VERT, SPAN(27, 12), SPAN(11, 12), SPAN(12, 18)+VERT, SPAN(13,  6)+VERT, SPAN(16, 12)+END,
  SPAN( 4, 14)+VERT, SPAN( 1,  4), SPAN( 2, 18)+VERT, SPAN( 3, 14)+VERT, SPAN(27,  8), SPAN(11, 14), SPAN(12, 18)+VERT, SPAN(13,  4)+VERT, SPAN(16, 14)+END,
  SPAN( 4, 16)+VERT, SPAN( 1,  2), SPAN( 2, 18)+VERT, SPAN( 3, 16)+VERT, SPAN(27,  4), SPAN(11, 16), SPAN(12, 18)+VERT, SPAN(13,  2)+VERT, SPAN(16, 16)+END,
  SPAN( 4, 18)+VERT, SPAN( 2, 18)+VERT, SPAN( 3, 18)+VERT, SPAN(11, 18), SPAN(12, 18)+VERT, SPAN(16, 18)+END,
  SPAN( 4, 18)+VERT, SPAN( 5,  2)+VERT, SPAN( 2, 16), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12, 16)+VERT, SPAN(15,  2), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5,  4)+VERT, SPAN( 2, 14), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12, 14)+VERT, SPAN(15,  4), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5,  6)+VERT, SPAN( 2, 12), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12, 12)+VERT, SPAN(15,  6), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5,  8)+VERT, SPAN( 2, 10), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12, 10)+VERT, SPAN(15,  8), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5, 10)+VERT, SPAN( 2,  8), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12,  8)+VERT, SPAN(15, 10), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5, 12)+VERT, SPAN( 2,  6), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12,  6)+VERT, SPAN(15, 12), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5, 14)+VERT, SPAN( 2,  4), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12,  4)+VERT, SPAN(15, 14), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5, 16)+VERT, SPAN( 2,  2), SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(12,  2)+VERT, SPAN(15, 16), SPAN(16, 18)+VERT+END,
  SPAN( 4, 18)+VERT, SPAN( 5, 18)+VERT, SPAN( 3, 18)+VERT, SPAN(11, 18)+VERT, SPAN(15, 18), SPAN(16, 18)+VERT+END,
  SPAN( 7,  2)+VERT, SPAN( 4, 16), SPAN( 5, 18)+VERT, SPAN( 6,  2)+VERT, SPAN( 3, 16), SPAN(11, 16)+VERT, SPAN(14,  2), SPAN(15, 18)+VERT, SPAN(16, 16)+VERT, SPAN(19,  2)+END,
  SPAN( 7,  4)+VERT, SPAN( 4, 14), SPAN( 5, 18)+VERT, SPAN( 6,  4)+VERT, SPAN( 3, 14), SPAN(11, 14)+VERT, SPAN(14,  4), SPAN(15, 18)+VERT, SPAN(16, 14)+VERT, SPAN(19,  4)+END,
  SPAN( 7,  6)+VERT, SPAN( 4, 12), SPAN( 5, 18)+VERT, SPAN( 6,  6)+VERT, SPAN( 3, 12), SPAN(11, 12)+VERT, SPAN(14,  6), SPAN(15, 18)+VERT, SPAN(16, 12)+VERT, SPAN(19,  6)+END,
  SPAN( 7,  8)+VERT, SPAN( 4, 10), SPAN( 5, 18)+VERT, SPAN( 6,  8)+VERT, SPAN( 3, 10), SPAN(11, 10)+VERT, SPAN(14,  8), SPAN(15, 18)+VERT, SPAN(16, 10)+VERT, SPAN(19,  8)+END,
  SPAN( 7, 10)+VERT, SPAN( 4,  8), SPAN( 5, 18)+VERT, SPAN( 6, 10)+VERT, SPAN( 3,  8), SPAN(11,  8)+VERT, SPAN(14, 10), SPAN(15, 18)+VERT, SPAN(16,  8)+VERT, SPAN(19, 10)+END,
  SPAN( 7, 12)+VERT, SPAN( 4,  6), SPAN( 5, 18)+VERT, SPAN( 6, 12)+VERT, SPAN( 3,  6), SPAN(11,  6)+VERT, SPAN(14, 12), SPAN(15, 18)+VERT, SPAN(16,  6)+VERT, SPAN(19, 12)+END,
  SPAN( 7, 14)+VERT, SPAN( 4,  4), SPAN( 5, 18)+VERT, SPAN( 6, 14)+VERT, SPAN( 3,  4), SPAN(11,  4)+VERT, SPAN(14, 14), SPAN(15, 18)+VERT, SPAN(16,  4)+VERT, SPAN(19, 14)+END,
  SPAN( 7, 16)+VERT, SPAN( 4,  2), SPAN( 5, 18)+VERT, SPAN( 6, 16)+VERT, SPAN( 3,  2), SPAN(11,  2)+VERT, SPAN(14, 16), SPAN(15, 18)+VERT, SPAN(16,  2)+VERT, SPAN(19, 16)+END,
  SPAN( 7, 18)+VERT, SPAN( 5, 18)+VERT, SPAN( 6, 18)+VERT, SPAN(14, 18), SPAN(15, 18)+VERT, SPAN(19, 18)+END,
  SPAN( 7, 18)+VERT, SPAN( 8,  2)+VERT, SPAN( 5, 16), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15, 16)+VERT, SPAN(18,  2), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8,  4)+VERT, SPAN( 5, 14), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15, 14)+VERT, SPAN(18,  4), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8,  6)+VERT, SPAN( 5, 12), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15, 12)+VERT, SPAN(18,  6), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8,  8)+VERT, SPAN( 5, 10), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15, 10)+VERT, SPAN(18,  8), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8, 10)+VERT, SPAN( 5,  8), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15,  8)+VERT, SPAN(18, 10), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8, 12)+VERT, SPAN( 5,  6), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15,  6)+VERT, SPAN(18, 12), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8, 14)+VERT, SPAN( 5,  4), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15,  4)+VERT, SPAN(18, 14), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8, 16)+VERT, SPAN( 5,  2), SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(15,  2)+VERT, SPAN(18, 16), SPAN(19, 18)+VERT+END,
  SPAN( 7, 18)+VERT, SPAN( 8, 18)+VERT, SPAN( 6, 18)+VERT, SPAN(14, 18)+VERT, SPAN(18, 18), SPAN(19, 18)+VERT+END,
  SPAN(31,  2), SPAN( 7, 16), SPAN( 8, 18)+VERT, SPAN( 9,  2)+VERT, SPAN( 6, 16), SPAN(14, 16)+VERT, SPAN(17,  2), SPAN(18, 18)+VERT, SPAN(19, 16)+VERT+END,
  SPAN(31,  4), SPAN( 7, 14), SPAN( 8, 18)+VERT, SPAN( 9,  4)+VERT, SPAN( 6, 14), SPAN(14, 14)+VERT, SPAN(17,  4), SPAN(18, 18)+VERT, SPAN(19, 14)+VERT+END,
  SPAN(31,  6), SPAN( 7, 12), SPAN( 8, 18)+VERT, SPAN( 9,  6)+VERT, SPAN( 6, 12), SPAN(14, 12)+VERT, SPAN(17,  6), SPAN(18, 18)+VERT, SPAN(19, 12)+VERT+END,
  SPAN(31,  8), SPAN( 7, 10), SPAN( 8, 18)+VERT, SPAN( 9,  8)+VERT, SPAN( 6, 10), SPAN(14, 10)+VERT, SPAN(17,  8), SPAN(18, 18)+VERT, SPAN(19, 10)+VERT+END,
  SPAN(31, 10), SPAN( 7,  8), SPAN( 8, 18)+VERT, SPAN( 9, 10)+VERT, SPAN( 6,  8), SPAN(14,  8)+VERT, SPAN(17, 10), SPAN(18, 18)+VERT, SPAN(19,  8)+VERT+END,
  SPAN(31, 12), SPAN( 7,  6), SPAN( 8, 18)+VERT, SPAN( 9, 12)+VERT, SPAN( 6,  6), SPAN(14,  6)+VERT, SPAN(17, 12), SPAN(18, 18)+VERT, SPAN(19,  6)+VERT+END,
  SPAN(31, 14), SPAN( 7,  4), SPAN( 8, 18)+VERT, SPAN( 9, 14)+VERT, SPAN( 6,  4), SPAN(14,  4)+VERT, SPAN(17, 14), SPAN(18, 18)+VERT, SPAN(19,  4)+VERT+END,
  SPAN(31, 16), SPAN( 7,  2), SPAN( 8, 18)+VERT, SPAN( 9, 16)+VERT, SPAN( 6,  2), SPAN(14,  2)+VERT, SPAN(17, 16), SPAN(18, 18)+VERT, SPAN(19,  2)+VERT+END,
  SPAN(31, 18), SPAN( 8, 18)+VERT, SPAN( 9, 18)+VERT, SPAN(17, 18), SPAN(18, 18)+VERT+END,
  SPAN(31, 20), SPAN( 8, 16), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18, 16)+VERT+END,
  SPAN(31, 22), SPAN( 8, 14), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18, 14)+VERT+END,
  SPAN(31, 24), SPAN( 8, 12), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18, 12)+VERT+END,
  SPAN(31, 26), SPAN( 8, 10), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18, 10)+VERT+END,
  SPAN(31, 28), SPAN( 8,  8), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18,  8)+VERT+END,
  SPAN(31, 30), SPAN( 8,  6), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18,  6)+VERT+END,
  SPAN(31, 32), SPAN( 8,  4), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18,  4)+VERT+END,
  SPAN(31, 34), SPAN( 8,  2), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT, SPAN(18,  2)+VERT+END,
  SPAN(31, 36), SPAN( 9, 18)+VERT, SPAN(17, 18)+VERT+END,
  SPAN(31, 38), SPAN( 9, 16), SPAN(17, 16)+VERT+END,
  SPAN(31, 40), SPAN( 9, 14), SPAN(17, 14)+VERT+END,
  SPAN(31, 42), SPAN( 9, 12), SPAN(17, 12)+VERT+END,
  SPAN(31, 44), SPAN( 9, 10), SPAN(17, 10)+VERT+END,
  SPAN(31, 46), SPAN( 9,  8), SPAN(17,  8)+VERT+END,
  SPAN(31, 48), SPAN( 9,  6), SPAN(17,  6)+VERT+END,
  SPAN(31, 50), SPAN( 9,  4), SPAN(17,  4)+VERT+END,
  SPAN(31, 52), SPAN( 9,  2), SPAN(17,  2)+VERT+END,
  SPAN(31, 54)+END,
  END,
}; // 1350 bytes

// Defines the faces that are highlighted to draw the "Kubic's Rube" font
#define FACEBITS(_ch,   \
            _23,        \
        _22,    _26,    \
     _21,   _25,   _29, \
        _24,    _28,    \
   _01,     _27,    _13,\
      _02,       _12,   \
   _04,  _03, _11,  _16,\
      _05,       _15,   \
   _07,  _06, _14,  _19,\
      _08,       _18,   \
         _09, _17)      \
    (dword)(((dword)(_23) << 23) | ((dword)(_22) << 22) | ((dword)(_26) << 26) | ((dword)(_21) << 21) | ((dword)(_25) << 25) | ((dword)(_29) << 29) | ((dword)(_24) << 24) | \
            ((dword)(_28) << 28) | ((dword)(_27) << 27) | ((dword)(_01) <<  1) | ((dword)(_13) << 13) | ((dword)(_02) <<  2) | ((dword)(_12) << 12) | ((dword)(_04) <<  4) | \
            ((dword)(_03) <<  3) | ((dword)(_11) << 11) | ((dword)(_16) << 16) | ((dword)(_05) <<  5) | ((dword)(_15) << 15) | ((dword)(_07) <<  7) | ((dword)(_06) <<  6) | \
            ((dword)(_14) << 14) | ((dword)(_19) << 19) | ((dword)(_08) <<  8) | ((dword)(_18) << 18) | ((dword)(_09) <<  9) | ((dword)(_17) << 17))
#define X 1
const dword PROGMEM fontFaces[] =
{
  FACEBITS('0',         \
             0,         \
         0,      0,     \
      0,     X,     0,  \
         X,      X,     \
     0,      0,      0, \
        X,        X,    \
     0,    0,  0,    0, \
        X,        X,    \
     0,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('1',         \
             0,         \
         0,      0,     \
      0,     X,     0,  \
         0,      0,     \
     0,      X,      0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
           0,  X        ),
  FACEBITS('2',         \
             X,         \
         X,      X,     \
      0,     0,     X,  \
         0,      X,     \
     0,      X,      0, \
        0,        0,    \
     0,    X,  0,    0, \
        X,        0,    \
     0,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('3',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     0,      0,      X, \
        0,        X,    \
     0,    X,  X,    X, \
        0,        0,    \
     0,    0,  0,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('4',         \
             0,         \
         X,      0,     \
      0,     X,     0,  \
         0,      X,     \
     X,      0,      X, \
        X,        X,    \
     0,    X,  X,    0, \
        0,        X,    \
     0,    0,  0,    0, \
        0,        X,    \
           0,  0        ),
  FACEBITS('5',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         X,      0,     \
     0,      X,      0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
     X,    0,  X,    0, \
        X,        0,    \
           X,  X        ),
  FACEBITS('6',         \
             X,         \
         X,      X,     \
      X,     0,     0,  \
         X,      0,     \
     X,      X,      0, \
        0,        0,    \
     X,    0,  X,    0, \
        0,        0,    \
     X,    0,  X,    0, \
        X,        0,    \
           X,  X        ),
  FACEBITS('7',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     0,      0,      0, \
        0,        X,    \
     0,    0,  0,    0, \
        0,        X,    \
     0,    0,  0,    0, \
        0,        X,    \
           0,  0        ),
  FACEBITS('8',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         X,      X,     \
     0,      X,      0, \
        X,        X,    \
     0,    0,  0,    0, \
        X,        X,    \
     0,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('9',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         X,      X,     \
     0,      X,      X, \
        0,        0,    \
     0,    0,  0,    X, \
        0,        0,    \
     0,    0,  0,    X, \
        0,        X,    \
           0,  0        ),

  FACEBITS('A',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        X,        X,    \
     X,    X,  X,    X, \
        0,        0,    \
           0,  0        ),  
  FACEBITS('B',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      X,     \
     X,      X,      0, \
        0,        0,    \
     X,    0,  X,    0, \
        0,        X,    \
     X,    0,  0,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('C',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      0, \
        0,        0,    \
     X,    0,  0,    0, \
        0,        0,    \
     X,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('D',         \
             X,         \
         X,      X,     \
      0,     0,     X,  \
         X,      0,     \
     0,      0,      X, \
        X,        0,    \
     0,    0,  0,    X, \
        X,        0,    \
     0,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('E',         \
             X,         \
         X,      X,     \
      X,     0,     0,  \
         0,      0,     \
     X,      0,      0, \
        X,        0,    \
     X,    X,  X,    0, \
        0,        0,    \
     X,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('F',         \
             X,         \
         X,      X,     \
      X,     0,     0,  \
         0,      0,     \
     X,      0,      0, \
        X,        0,    \
     X,    X,  X,    0, \
        0,        0,    \
     X,    0,  0,    0, \
        0,        0,    \
           0,  0        ),
  FACEBITS('G',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      0, \
        0,        X,    \
     X,    0,  X,    0, \
        0,        X,    \
     X,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
  FACEBITS('H',         \
             0,         \
         0,      0,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        X,        X,    \
     X,    X,  X,    X, \
        0,        0,    \
           0,  0        ),  
  FACEBITS('I',         \
             X,         \
         0,      0,     \
      0,     X,     0,  \
         0,      0,     \
     0,      X,      0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
           0,  X        ),
  FACEBITS('J',         \
             X,         \
         0,      X,     \
      0,     0,     X,  \
         0,      0,     \
     0,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        0,        0,    \
     X,    0,  0,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('K',         \
             X,         \
         X,      0,     \
      X,     0,     X,  \
         0,      X,     \
     X,      X,      0, \
        X,        0,    \
     X,    0,  0,    0, \
        0,        0,    \
     X,    X,  0,    X, \
        0,        X,    \
           X,  X        ),
  FACEBITS('L',         \
             X,         \
         X,      0,     \
      X,     0,     0,  \
         0,      0,     \
     X,      0,      0, \
        0,        0,    \
     X,    0,  0,    0, \
        0,        0,    \
     X,    0,  0,    0, \
        X,        0,    \
           X,  0        ),
  FACEBITS('M',         \
             X,         \
         X,      X,     \
      X,     X,     X,  \
         0,      0,     \
     X,      X,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        0,        0,    \
     X,    0,  0,    X, \
        0,        0,    \
           0,  0        ),
  FACEBITS('N',         \
             0,         \
         0,      X,     \
      X,     0,     X,  \
         X,      0,     \
     X,      X,      X, \
        0,        0,    \
     X,    0,  X,    X, \
        0,        X,    \
     X,    0,  0,    X, \
        0,        0,    \
           0,  0        ),
  FACEBITS('O',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        0,        0,    \
     X,    0,  0,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('P',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        X,        X,    \
     X,    X,  X,    0, \
        0,        0,    \
           0,  0        ),
  FACEBITS('Q',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        0,        0,    \
     X,    0,  X,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('R',         \
             X,         \
         X,      X,     \
      X,     0,     X,  \
         X,      X,     \
     X,      X,      0, \
        X,        0,    \
     X,    0,  0,    0, \
        0,        0,    \
     X,    X,  0,    0, \
        0,        X,    \
           0,  X        ),
  FACEBITS('S',         \
             X,         \
         X,      X,     \
      X,     0,     0,  \
         X,      0,     \
     0,      X,      0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
     X,    0,  X,    0, \
        X,        0,    \
           X,  X        ),
  FACEBITS('T',         \
             X,         \
         X,      X,     \
      X,     X,     X,  \
         0,      0,     \
     0,      X,      0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
           0,  X        ),
  FACEBITS('U',         \
             0,         \
         0,      0,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  0,    X, \
        0,        0,    \
     X,    0,  0,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('V',         \
             0,         \
         0,      0,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     0,    0,  0,    0, \
        X,        X,    \
     0,    0,  0,    0, \
        0,        0,    \
           X,  X        ),
  FACEBITS('W',         \
             0,         \
         0,      0,     \
      X,     0,     X,  \
         0,      0,     \
     X,      0,      X, \
        0,        0,    \
     X,    0,  X,    X, \
        0,        0,    \
     X,    0,  X,    X, \
        X,        X,    \
           X,  X        ),
  FACEBITS('X',         \
             0,         \
         0,      0,     \
      X,     0,     X,  \
         X,      X,     \
     0,      X,      0, \
        0,        0,    \
     0,    X,  X,    0, \
        X,        X,    \
     X,    0,  0,    X, \
        0,        0,    \
           0,  0        ),
  FACEBITS('Y',         \
             0,         \
         0,      0,     \
      X,     0,     X,  \
         X,      X,     \
     0,      X,      0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
     0,    0,  X,    0, \
        0,        0,    \
           0,  X        ),
  FACEBITS('Z',         \
             X,         \
         X,      X,     \
      0,     0,     X,  \
         0,      X,     \
     0,      X,      0, \
        0,        0,    \
     0,    X,  0,    0, \
        X,        0,    \
     X,    0,  0,    0, \
        X,        X,    \
           X,  X        ),
/*           
  FACEBITS(' ',         \
             0,         \
         0,      0,     \
      0,     0,     0,  \
         0,      0,     \
     0,      0,      0, \
        0,        0,    \
     0,    0,  0,    0, \
        0,        0,    \
     0,    0,  0,    0, \
        0,        0,    \
           0,  0        ),
*/           
};

void Cubes::DrawBackground()
{
  ILI948x::ColourByte(0x00, ILI948x::Window(0, 0, LCD_WIDTH, LCD_HEIGHT));
  for (int cube = 0; cube < 4; cube++)
    currentDigits[cube] = -1;
}


void Cubes::DrawDate()
{
  int rows = 3*(CubeHeight + 2*CubeStepUp);
  int cols = 6*CubeWidth;
  int x = 0;
  int dX = cols/2 + 3;
  int dY = 3*CubeHeight/2;  // stagger down?
  char buff[3];
  clock.GetNumStr(rtc.m_DayOfMonth, buff, true);
  x = DrawSmallCubes(x, 0, dX, dY, clock.GetDayName(rtc.m_DayOfWeek), 3) + dX/5;
  x = DrawSmallCubes(x, 0, dX, dY, buff, 2) + dX/5;
  DrawSmallCubes(x, 0, dX, dY, clock.GetMonthName(rtc.m_Month), 3);

  config.UpdateBin();
  if (config.binCalcNext != Config::eNoBin)
  {
    char daysCh = '0' + config.binCalcDaysToNext;
    word colour = (config.binCalcNext == Config::eYellowBin)?yellowBin:redBin;
    DrawCube((LCD_WIDTH - cols/2)/2, dY?dY*3 + 2*dY/3:3*rows/4, GetFontFaces(daysCh), true, colour);
  }
  
  currentDate = rtc.m_DayOfMonth;
}

#define TIME_Y ((LCD_HEIGHT - (3*(CubeHeight + 2*CubeStepUp)))/2 + 40)

void Cubes::DrawTime()
{
  int cols = 6*CubeWidth;
  int gap = LCD_WIDTH/4 - cols;
  int digits[4];
  clock.getTimeDigits(digits);
  for (int cube = 0; cube < 4; cube++)
  {
    if (currentDigits[cube] != digits[cube])
    {
      DrawCube(cube*LCD_WIDTH/4 + ((cube > 1)?gap:0), TIME_Y, GetFontFaces('0' + digits[cube]));
    }
    currentDigits[cube] = digits[cube];
  }
  currentMinute = rtc.m_Minute;
}

void Cubes::CheckUpdate()
{
  if (RTC::CheckPeriod(checkTimeMS, checkTimeDuration))
  {
    int thisMinute = rtc.ReadMinute();
    if (thisMinute != currentMinute)
    {
      DrawTime();
      if (currentDate != rtc.m_DayOfMonth)
      {
        DrawDate();
      }
    }
  }
  if (RTC::CheckPeriod(blinkTimeMS, blinkDuration))
  {
    blinkOn = !blinkOn;
    DrawCubelet(LCD_WIDTH/2 - CubeWidth/2, TIME_Y + CubeStepUp*3, blinkOn);
    DrawCubelet(LCD_WIDTH/2 - CubeWidth/2, TIME_Y + CubeStepUp*3 + CubeHeight*2, blinkOn);
  }
}

void Cubes::DrawCube(int x, int y, dword ch, bool half, word colour)
{
  int rows = 3*(CubeHeight + 2*CubeStepUp);
  int cols = 6*CubeWidth;
  word colours[32];
  int factor = (half)?2:1;

  for (int c = 1; c < 30; c++)
    colours[c] = GetRandomColour();

  colours[ 0] = colour;
  colours[30] = 0x0000;
  colours[31] = 0x0000;
  ILI948x::Window(x, y, cols/factor, rows/factor);

  const word* pSpan = faceSpans;
  for (int row = 0; row < rows; row++)
  {
    int remainder = cols/factor;
    bool skip = false;
    while (true)
    {
      word data = pgm_read_word(pSpan++);
      int faceNum = (data >> 8) & 0x001F;
      int count = (data & 0x00FF)/factor;
      
      word colour = colours[faceNum];
      dword faceBit = 1L << faceNum;
      if (ch & faceBit)
      {
        // face is part of the char definition, change colour
        colour = colours[0];
      }
      remainder -= count;

      byte lineColour = (colour == 0xFFFF)?0x00:0xFF;
      skip = row % factor; 
      if (!skip)
      {
        if (faceNum / 10 != 3)  // non-background
        {
          if (!half || count >= 2)  // put in detailed dotted lines between face
          {
            if ((data & VERT) && (row % (2*factor)))
            {
              ILI948x::ColourWord(colour, count - 1);
            }
            else
            {
              ILI948x::ColourByte(lineColour, 1); // lines between
              ILI948x::ColourWord(colour, count - 2);
            }
            
            if ((data & END) && (remainder || (row % (2*factor))))
              ILI948x::ColourByte(lineColour, 1); // lines between
            else        
              ILI948x::ColourWord(colour, 1);
          }
          else
          {            
              ILI948x::ColourByte(lineColour, count);
          }
        }
        else
        {
          ILI948x::ColourByte(0x00, count);
        }
      }
      if (data & END)
        break;
    }
    if (!skip)
    {
      ILI948x::ColourByte(0x00, remainder);
    }
  }    
}

void Cubes::DrawCubelet(int x, int y, bool on)
{
  // draw a single cube (the front centre one)

  word colours[32];
  int factor = 2;
  dword fill = ILI948x::Window(x, y, CubeWidth*2/factor, (CubeHeight + 2*CubeStepUp)/factor);
  if (!on)
  {
    ILI948x::ColourByte(0x00, fill);
    return;
  }

  // the face we draw in full:
  dword fullFaces = (dword)((1L << 27) | (1L << 11) | (1L << 3));
  // the faces we draw in full or in part, anyting else is ignored
  dword drawFaces = (dword)((1L << 28) |(1L << 24) | (1L << 14) | (1L << 6) | fullFaces);
  for (int c = 0; c < 32; c++)
    if ((1L << c) & fullFaces)
      colours[c] = ByteHSVtoRGB(0, 0, VAL_Normal - 4*(32 - c)); // just shades of gray
      
  int rows = 3*(CubeHeight + 2*CubeStepUp);
  const word* pSpan = faceSpans;
  int partialTopRows = -1; // not started
  int partialSideRows = -1; // not started
  for (int row = 0; row < rows; row++)
  {
    while (true)
    {
      word data = pgm_read_word(pSpan++);
      int faceNum = (data >> 8) & 0x001F;
      dword faceBit = 1L << faceNum;
      if (!(row % factor) && (faceBit & drawFaces))
      {
        int count = (data & 0x00FF)/factor;
        word colour = colours[faceNum];
        if (faceBit & fullFaces)
        {
           // draw as normal
           ILI948x::ColourWord(colour, count);
        }
        else
        {
          if ((faceNum / 10) == 2)
          {
            // Partial top face.  Draw (black) only the lower StepUp rows and halve the width
            if (faceNum == 24)  // leftmost
            {
              if (partialTopRows == -1)
                partialTopRows = 1;
              else
                partialTopRows++;
            }
            if (partialTopRows > CubeStepUp/factor)
               ILI948x::ColourByte(0x00, count/2);
          }
          else
          {
            // Partial side face,  Draw (black) only the upper StepUp rows, full width
            if (faceNum == 6)  // leftmost
            {
              if (partialSideRows == -1)
                partialSideRows = 1;
              else
                partialSideRows++;
            }
            if (partialSideRows != -1 && partialSideRows <= CubeStepUp/factor)
               ILI948x::ColourByte(0x00, count);
          }
        }
      }
      
      if (data & END)
        break;
    }
  }  
}

dword Cubes::GetFontFaces(char ch)
{
  int index = 0;
  if ('0' <= ch && ch <= '9')
    index = ch - '0';
  else if ('A' <= ch && ch <= 'Z')
    index = ch - 'A' + 10;
  else
    return 0L;
  return pgm_read_dword(fontFaces + index);
}

word Cubes::GetRandomColour()
{
    byte randColourH = random(0x100);         // random Hue 0..255
    byte randColourS = random(0x7F, 0x100);  // random Saturation 50..99, or 75..99?, 0xBFFF
    // random value 50..(100-MaxLightening)
    int maxDeltaV = 0;
    byte randColourV = random(0x7F, 0xFF - maxDeltaV*2);
    return ByteHSVtoRGB(randColourH, randColourS, randColourV);
}

int Cubes::DrawSmallCubes(int x, int y, int dX, int dY, const char* str, int len)
{
  for (int ch = 0; ch < len; ch++)
  {
    DrawCube(x, y, GetFontFaces(str[ch]), true);
    x += dX;
    y += dY;
  }
  return x;
}
