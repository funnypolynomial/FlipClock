#pragma once

// Orientation is Landscape right (USB bottom right) UNLESS one of these is defined:
//#define LCD_PORTRAIT_BOT    // Portrait, USB connector at bottom
//#define LCD_PORTRAIT_TOP    // Portrait, USB connector at top
//#define LCD_LANDSCAPE_LEFT  // Landscape, USB connector at top left

// Try defining this if the display is jumbled. Only tested for Landscape
//#define LCD_FIXED_ORIGIN

// optionally dump graphics cmds to serial:
//#define SERIALIZE
#ifdef SERIALIZE
#define SERIALISE_ON(_on) ILI948x::_serialise=_on;
#define SERIALISE_COMMENT(_c) if (ILI948x::_serialise) { Serial.print("; ");Serial.println(_c);}
#else
#define SERIALISE_ON(_on)
#define SERIALISE_COMMENT(_c)
#endif

#if defined(LCD_PORTRAIT_BOT) || defined(LCD_PORTRAIT_TOP)
#define LCD_WIDTH  320
#define LCD_HEIGHT 480
#else               
#define LCD_WIDTH  480
#define LCD_HEIGHT 320
#endif

#define LCD_RD_BIT   B00000001  // A0
#define LCD_WR_BIT   B00000010
#define LCD_RS_BIT   B00000100
#define LCD_CS_BIT   B00001000
#define LCD_RST_BIT  B00010000  // A4

#define LCD_RD_PIN   A0
#define LCD_WR_PIN   A1     
#define LCD_RS_PIN   A2        
#define LCD_CS_PIN   A3       
#define LCD_RST_PIN  A4
// BUTTON            A5

// SD card not used
#define SD_SCK_PIN   13 // LED_BUILTIN
#define SD_DO_PIN    12 // SCL
#define SD_DI_PIN    11 // SDA
#define SD_SS_PIN    10 // BUTTON

#define RGB(_r, _g, _b) (word)((_b & 0x00F8) >> 3) | ((_g & 0x00FC) << 3) | ((_r & 0x00F8) << 8)

class ILI948x
{
  public:
    static void Init();
    static void DisplayOn();
    static void Cmd(byte cmd);
    static void DataByte(byte data);
    static void DataWord(word data);
    static void ColourWord(word colour, unsigned long count);
    static void ColourByte(byte colour, unsigned long count);
    static unsigned long Window(word x,word y,word w,word h);
    static void OneWhite();
    static void OneBlack();

    static void SetScroll(bool left); // portrait direction
    static void Scroll(uint16_t cols);
    static byte m_MADCTL0x36;

#ifdef SERIALIZE
     static bool _serialise;
#endif
    
private:
    static const byte initialisation[];
};
