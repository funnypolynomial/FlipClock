#include <Arduino.h>
#include <avr/pgmspace.h>
#include "TriangleMesh.h"
#include "ILI948x.h"
#include "HSV.h"


// ADAPTED FROM HSVClock

// Triangle Mesh:
//  Start with a grid of square cells.  
//  Display characters using a simple 4x4 font by turning cells "on" or "off".
//  Then randomly shift the top left corner of each cell a little in X & Y.
//  (Distort less at the edges of the grid and where characters are displayed.)
//  Divide the cell (now a quadrilateral) into two triangles with a random diagonal.
//  When painting the cells, start with a random base colour as HSV (Hue Saturation and Value).
//  Change the hue by a random amount across and down the grid.
//  Shade the triangular facets of the quadrilateral by altering the Value so the triangles look lit from top left.
//  Highlight the "on" cells by taking the hue the cell would have and using the complimentary hue instead.
//  
//  A redraw of the cells is either
//  1: A full redraw: a left-to-right-and-down scan of every cell, required when the grid has been randomised.  
//  The exposed black areas around the edges are dealt with separately.
//  2: A partial redraw: each cell is repainted, done when the colours are re-randomized.  
//  A Linear feedback shift register is used to visit every cell in a random order, rather than a scan.
//  The exposed black areas around the edges remaind untouched.
//  3: possibly an individual cell (and it's neighbours) are redrawn in response to a touch.
//  
//  The quadrilaterals are drawn as two triangles by evaluating Bresenham's line algorithm for each line.
//  Instead of drawing within the algorithm, arrays of left-most and right-most X coordinates, for a given Y, are updated.
//  These are then used to draw a series of horizontal lines on the lcd.
//  
//  fastTFT is designed to draw these lines as fast as possible.


#define swap(_a, _b) {long t=_a;_a=_b;_b=t;}

// interface to ILI948x
word fastTFT_rgb1;
word fastTFT_rgb2;
// window with top-left at (X,Y), across to X2 and to the bottom of the screen
#define fastTFT_Window(_X, _Y, _X2) ILI948x::Window(_X, _Y, (_X2) - (_X) + 1, screenHeight-(_Y) + 1);
#define fastTFT_BlackPixels(_c) ILI948x::ColourByte(0x00, _c);
// In HSV clock we pre-computed the components of the two colours because we were switching, don't currently bother
#define fastTFT_SetColour(_rgb1) fastTFT_rgb1 = _rgb1;
#define fastTFT_SetAltColour(_rgb2) fastTFT_rgb2 = _rgb2;
#define fastTFT_ColourPixels(_c) ILI948x::ColourWord(fastTFT_rgb1, _c);
#define fastTFT_AltColourPixels(_c) ILI948x::ColourWord(fastTFT_rgb2, _c);

// simple 4x5 cell character definitions, packed into DWORDs
// 5 nibbles, each nibble is a row, most significant bit is left-most cell
const  unsigned long TriangleMesh::charDefs[] PROGMEM = 
{
  // '/'= thin '1', sans serifs RHS
  0x44444,
  // 0..9
  0x75557, 0x62227,0x71747,0x71717,0x55711,0x74717, 0x74757,0x71111,0x75757,0x75717,
  // ':'=':'
  0x04040,
  // ';'=<degrees>
  0x75700,
  // '<'= thin '1', centred
  0x22222
};


TriangleMesh::TriangleMesh():
  flags(0)
{
}

TriangleMesh::~TriangleMesh()
{
}

void TriangleMesh::Clear()
{
  ClearCellStates();
}

// display the given string
int TriangleMesh::Display(const char* Str, int row, int col)
{
  while (*Str && col < cellsAcross)
  {
    SetChar(GetCharDefinition(*Str), row, col);
    Str++;
  }
  return col;
}

// repaint the whole grid
void TriangleMesh::Paint()
{
  // rasterizing of triangles (in pairs)
  char aX[maxVertexY];
  char bX[maxVertexY];
  char cX[maxVertexY];

  // init each row to N/A
  for (int i = 0; i < maxVertexY; i++)
  {
    aX[i] = bX[i] = cX[i] = vertexNULL;
  }

  if (flags & newMeshFlag)
  {
    // blank around the outside, could be smarter and less noticable, but would complicate & slow the code
    // blank upper
    fastTFT_Window(0, 0, screenWidth - 1);
    fastTFT_BlackPixels(screenWidth*(edgeHeight + maxVertexOffset));

    // blank left
    fastTFT_Window(0, edgeHeight + maxVertexOffset, edgeWidth + maxVertexOffset - 1);
    fastTFT_BlackPixels((edgeWidth + maxVertexOffset)*(screenHeight - 2*(edgeHeight + maxVertexOffset)));
    
    // blank right
    fastTFT_Window(screenWidth - edgeWidth - maxVertexOffset, edgeHeight + maxVertexOffset, screenWidth - 1);
    fastTFT_BlackPixels((edgeWidth + maxVertexOffset)*(screenHeight - 2*(edgeHeight + maxVertexOffset)));

    // blank lower
    fastTFT_Window(0, screenHeight - edgeHeight - maxVertexOffset, screenWidth - 1);
    fastTFT_BlackPixels(screenWidth*(edgeHeight + maxVertexOffset));

    // paint the cells left to right and top down
    for (int row = 0; row < cellsDown; row++)
    {
      for (int col = 0; col < cellsAcross; col++)
      {
        PaintCell(col, row, aX, bX, cX);
      }
    }
    flags &= ~newMeshFlag;
  }
  else if (flags & blinkColonFlag)
  {
    // blink the colon, a bit of a hack the mesh knows where they are :-)
    PaintCell(7, 4, aX, bX, cX);
    PaintCell(7, 6, aX, bX, cX);
    flags &= ~blinkColonFlag;
    flags &= ~blinkColonOffFlag;
  }
  else
  {
    // recolour the existing cells, visit the cells in a pseudo-random order
    // http://en.wikipedia.org/wiki/Linear_feedback_shift_register
    byte LFSR = (byte)cellColourS;
    if (!LFSR) 
    {
      // seed must be non-0
      LFSR = 0xCA;
    }
    for (int i = 0; i < 255; i++)
    {
      // taps @ 8 6 5 4, period 255
      byte bit  = ((LFSR >> 0) ^ (LFSR >> 2) ^ (LFSR >> 3) ^ (LFSR >> 4) ) & 1;
      LFSR = (LFSR >> 1) | (bit << 7);
      if (LFSR <= cellsAcross*cellsDown)
      {
        // we see LFSR in the range 1..cellsAcross*cellsDown
        int col = (LFSR % cellsAcross);
        int row = (LFSR / cellsAcross) % cellsDown;
        PaintCell(col, row, aX, bX, cX);
      }
    }
  }
  SERIALISE_COMMENT("*** DONE");
}

void TriangleMesh::BlinkColon(bool On)
{
  flags |= blinkColonFlag;
  if (!On)
    flags |= blinkColonOffFlag;
  Paint();
}



// paint the given cell
void TriangleMesh::PaintCell(int col, int row, char* aX, char* bX, char* cX)
{
  int x0 = col*cellWidth  - maxVertexOffset;
  int y0 = row*cellHeight - maxVertexOffset;

  int x1,y1,x2,y2,x3,y3,x4,y4;  // 1 2
                                // 4 3

  GetVertex(x0, y0, col,   row,   x1, y1);
  GetVertex(x0, y0, col+1, row,   x2, y2);
  GetVertex(x0, y0, col+1, row+1, x3, y3);
  GetVertex(x0, y0, col,   row+1, x4, y4);

  // don't overlap the cells
  x2--;
  y4--;
  x3--;
  y3--;

  byte cell = cells[col][row];
  bool on = (cell & cellStateBit) != 0;
  bool diagonal = (cell & cellDiagonalBit) != 0;

  Colour rgb1;
  Colour rgb2;
  GetCellColours(col, row, on, diagonal, rgb1, rgb2);

  // compute the rows of pixels
  if (diagonal)  // diagonal is '/'
  {
    // 1 2
    // 4
    Bresenham(x1, y1, x2, y2, aX, bX);
    Bresenham(x2, y2, x4, y4, NULL, bX);  // the diagonal can't have left-most pixels
    Bresenham(x4, y4, x1, y1, aX, NULL);  // the vertical can't have right-most pixels

    //   2
    // 4 3
    Bresenham(x2, y2, x3, y3, NULL, cX);  // the vertical can't have left-most pixels
    Bresenham(x3, y3, x4, y4, bX, cX);
    // (don't do the diagonal again)
  }
  else
  {
    // "left" triangle before "right" one; swap colour
    swap(rgb1, rgb2);

    // 1
    // 4 3
    Bresenham(x1, y1, x3, y3, NULL, bX);
    Bresenham(x3, y3, x4, y4, aX, bX);
    Bresenham(x4, y4, x1, y1, aX, NULL);

    // 1 2
    //   3
    Bresenham(x1, y1, x2, y2, bX, cX);
    Bresenham(x2, y2, x3, y3, NULL, cX);
  }

  // draw the lines
  int startX = x0 + edgeWidth;
  int startY = y0 + edgeHeight;
  fastTFT_SetColour(rgb1);
  fastTFT_SetAltColour(rgb2);

  for (int i = 0; i < maxVertexY; i++)
  {
    if (aX[i] != vertexNULL) // there's a LHS
    {
      fastTFT_Window(startX + aX[i], startY, screenWidth);
      fastTFT_ColourPixels(bX[i] - aX[i] + 1);

      if (cX[i] > bX[i])
      {
        // there's a RHS and it's more than 1 pixel
        // let the LHS have the shared pixel
        fastTFT_AltColourPixels(cX[i] - bX[i]);
      }
      // clear as we go
      aX[i] = bX[i] = cX[i] = vertexNULL;
    }
    else if (cX[i] != vertexNULL) // there's just a RHS
    {
      fastTFT_Window(startX + bX[i], startY, screenWidth);
      fastTFT_AltColourPixels(cX[i] - bX[i] + 1);
      // clear as we go
      bX[i] = cX[i] = vertexNULL;
    }

    startY++;
  }
}

Colour TriangleMesh::HSVtoRGB(ColourComponent h, ColourComponent s, ColourComponent v)
{
  return ::HSVtoRGB(h, s, v);         
}

// randomize the colours across the mesh
void TriangleMesh::RandomizeColour(bool smear)
{
  if (smear)
    flags |= smearColoursFlag;
  else
    flags &= ~smearColoursFlag;
  // base seed colour
  cellColourH = random(0x10000);      // random Hue 0..359
  cellColourS = random(0x7FFF, 0x10000);  // random Saturation 50..99, or 75..99?, 0xBFFF
  // random value 50..(100-MaxLightening)
  cellColourV = random(0x7FFF, 0xFFFF - maxDeltaV*2);

  // the delta hue per column, -maxDeltaH..maxDeltaH
  cellDeltaHCol = maxDeltaH - random(2*maxDeltaH);
  // the delta hue per row, -maxDeltaH..maxDeltaH
  cellDeltaHRow = maxDeltaH - random(2*maxDeltaH);
}

// get the two colours for the triangles in the cell
void TriangleMesh::GetCellColours(int col, int row, bool on, bool diagonal, Colour& rgb1, Colour& rgb2)
{
  ColourComponent h = cellColourH;
  ColourComponent s = cellColourS;
  // shade the triangle faces
  ColourComponent V1 = cellColourV + maxDeltaV/((diagonal)?1:2);
  ColourComponent V2 = cellColourV - maxDeltaV/((diagonal)?1:2);

  if (flags & smearColoursFlag)
  {
    h += (ColourComponent)(row*cellDeltaHRow + col*cellDeltaHCol);
  }

  bool colonOffHack = (flags & blinkColonOffFlag);
  if (on && !colonOffHack)
  {
    // make the on cells stand out, use the complimentary colour (unless we're hiding the colon!)
    h += 0x7FFF;
  }

  rgb1 = HSVtoRGB(h, s, V1);
  rgb2 = HSVtoRGB(h, s, V2);
}

// virtual, returns the 3x5 font definition for Char, or 0
unsigned long TriangleMesh::GetCharDefinition(char Char)
{
  if ('/' <= Char && Char <= '<')
  {
    return   pgm_read_dword(charDefs + Char - '/');
  }
  return 0x00000;
}

// set the cell states to display the character definition at the position on the grid
void TriangleMesh::SetChar(unsigned long charDef, int baseRow, int& baseCol)
{
  unsigned long Mask = 0x100000;
  for (int Y = 0; Y < fontHeight; Y++)
    for (int X = 0; X < fontWidth; X++)
      if (charDef & (Mask >>= 1))
        cells[baseCol + X][baseRow - fontHeight + Y + 1] |= cellStateBit;
      else
        cells[baseCol + X][baseRow - fontHeight + Y + 1] &= ~cellStateBit;

  baseCol += fontWidth;
}

// zero the states of all the cells
void TriangleMesh::ClearCellStates()
{
  for (int row = 0; row <= cellsDown; row++)
  {
    for (int col = 0; col <= cellsAcross; col++)
    {
      // DXXXSYYY, clear S
      cells[col][row] &= ~cellStateBit;
    }
  }
}

// randomize (distort) the mesh
void TriangleMesh::RandomizeMesh(bool distort)
{
  if (distort)
    flags |= distortVerticesFlag;
  else
    flags &= ~distortVerticesFlag;
  for (int row = 0; row <= cellsDown; row++)
  {
    for (int col = 0; col <= cellsAcross; col++)
    {
      RandomizeCell(cells[col][row]);
    }
  }
  flags |= newMeshFlag;
}


// distort the cell corner at the position
bool TriangleMesh::RandomizeNode(int x, int y)
{
  // only if a redraw is fast enough
  static int prevCol = -1;
  static int prevRow = -1;
  int col = x / cellWidth;
  int row = y / cellHeight;
  if (0 <= col && col < cellsAcross && 0 <= row && row < cellsDown)
  {
    if (row != prevRow || col != prevCol)
    {
      prevRow = row;
      prevCol = col;
      RandomizeCell(cells[col][row]);
      return true;
    }
  }
  return false;
}

// randomize a cell's position and diagonal direction
void TriangleMesh::RandomizeCell(byte& cell)
{
  // DXXXSYYY
  cell &= cellStateBit;  // preserve the cell state (S)
  cell |= random() & ~cellStateBit; // random Diagonal, dX & dY
}

// get the amount of distortion encoded in the nibble, up to max
int TriangleMesh::CellOffset(byte nibble, int max)
{
  int offset = (nibble & 0x07) + 1;  // 1..8
  if (offset > maxVertexOffset)
  {
    // -1..-4
    offset = maxVertexOffset - offset;
    if (offset < -max)
    {
      offset = -max;
    }
  }
  else
  {
    // +1..+4
    if (offset > +max)
    {
      offset = +max;
    }
  }
  return offset;
}

// get the (top left) corner of the given cell, normalized relative to x0, y0
void TriangleMesh::GetVertex(int x0, int y0, int col, int row, int& x, int& y)
{
  x = col*cellWidth - x0;
  y = row*cellHeight - y0;
  if (flags & distortVerticesFlag)
  {
    byte cell = cells[col][row];
    int max = maxVertexOffset;
    
    if (4 <= row && row <= 9)
    {
      // On/Off cells are distorted less
      max >>= 1;
      if (cell & cellStateBit)
      {
        // On cells are distorted less still
        max >>= 1;
      }
    }

    if (0 < col && col < cellsAcross)
    {
      x += CellOffset(cell >> 4, max);
    }
    else
    {
      // edges are special cases, shift less to stay on screen
      x += CellOffset(cell >> 4, edgeWidth - 1);
    }

    if (0 < row && row < cellsDown)
    {
      y += CellOffset(cell, max);
    }
    else
    {
      // edges are special cases, shift less to stay on screen
      y += CellOffset(cell, edgeHeight - 1);
    }
  }
}

/*
       ____
    a-/    ----b___    <- a, b, triangle 1 only
     /   1   ___-- /
  a-/ ___-b--     /-c  <- a, b, c, triangle 1 & 2
   /-___     2   /
       b----____/-c    <- b, c, triangle 2 only
*/
// "virtually" render a line, updating the arrays of minimum and maximum x coordinates, normalised by x0, y0
void TriangleMesh::Bresenham(int x0, int y0,int x1, int y1, char* min, char* max)
{
  // based on http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep)
  {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1)
  {
    swap(x0, x1);
    swap(y0, y1);
  }
  int deltax = x1 - x0;
  int deltay = abs(y1 - y0);
  int error = deltax / 2;
  int ystep;
  int y = y0;
  if (y0 < y1)
    ystep = 1;
  else 
    ystep = -1;
  for (int x = x0; x <= x1; x++)
  {
    int plotX = x;
    int plotY = y;
    if (steep)
      swap(plotX,plotY);

    // This is where we would plot(plotX,plotY) if we were drawing the line.
    // just update the row info instead
    if (min)
    {
      if (min[plotY] == vertexNULL)
      {
        // first time, init the row
        min[plotY] = plotX;
      }
      else
      {
        // update min
        if (plotX < min[plotY])
          min[plotY] = plotX;
      }
    }

    if (max && plotX > max[plotY]) // update max
    {
      // Note: no need to check against vertexNULL, it's -99
      max[plotY] = plotX;
    }

    error -= deltay;
    if (error < 0)
    {
      y += ystep;
      error += deltax;
    }
  }
}
