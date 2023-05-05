#pragma once

typedef word Colour;

typedef word ColourComponent;

class TriangleMesh
{
public:
  TriangleMesh();
  virtual ~TriangleMesh();
  void Paint();

  void Clear();
  int  Display(const char* Str, int row, int col);
  void RandomizeColour(bool smear);
  void RandomizeMesh(bool distort);
  bool RandomizeNode(int x, int y);
  void BlinkColon(bool On);

private:
  void PaintCell(int col, int row, char* aX, char* bX, char* cX);
  Colour HSVtoRGB(ColourComponent h, ColourComponent s, ColourComponent v);
  void GetCellColours(int col, int row, bool on, bool diagonal, Colour& rgb1, Colour& rgb2);
  virtual unsigned long GetCharDefinition(char Char);
  void SetChar(unsigned long charDef, int baseRow, int& baseCol);
  void RandomizeCell(byte& cell);
  int  CellOffset(byte nibble, int max);
  void GetVertex(int x0, int y0, int col, int row, int& x, int& y);
  void ClearCellStates();
  void Bresenham(int x0,int y0,int x1,int y1, char* min, char* max);

  static  const unsigned long charDefs[] PROGMEM;

  // defines to save memory:
  #define fontWidth       (4)
  #define fontHeight      (5)
  #define screenWidth     (480)
  #define screenHeight    (320)
  #define cellWidth       (screenWidth/(fontWidth*4 + 1))
  #define cellHeight      (cellWidth)
  #define cellsAcross     (screenWidth/cellWidth)
  #define cellsDown       (screenHeight/cellHeight)
  #define edgeWidth       ((screenWidth - cellsAcross*cellWidth)/2)
  #define edgeHeight      ((screenHeight - cellsDown*cellHeight)/2)
  #define maxVertexOffset (4)
  #define maxVertexY      (cellHeight + 2*maxVertexOffset + 1)
  #define vertexNULL      (-99)
  #define maxDeltaV       ((ColourComponent)(13107)) // 20% of 65535
  #define maxDeltaH       ((ColourComponent)(3276)) // 5% of 65535
  #define cellStateBit        (0x08)  // cell is on
  #define cellDiagonalBit     (0x80)  // cell diagonal is '/'
  #define distortVerticesFlag (0x01)  // shift the vertices
  #define smearColoursFlag    (0x02)  // change the colour
  #define newMeshFlag         (0x04)  // vertices in new locations
  
  #define blinkColonFlag      (0x10)  // hack, blink the colon
  #define blinkColonOffFlag   (0x20)  // hack, blink the colon off (vs on)

  byte flags;
  ColourComponent cellColourH;
  ColourComponent cellColourS;
  ColourComponent cellColourV;
  int cellDeltaHCol;
  int cellDeltaHRow;

  // DXXXSYYY dX/DY+/-1..<maxVertexOffset>, D is diagonal, S is on/off
  byte cells[cellsAcross + 1][cellsDown + 1];
};

