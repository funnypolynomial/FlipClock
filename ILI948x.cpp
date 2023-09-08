#include <Arduino.h>
#include <avr/pgmspace.h>
#include "ILI948x.h"

#ifdef LCD_PORTRAIT_BOT
#define MADCTL0x36 B00001000
#elif defined LCD_PORTRAIT_TOP
#define MADCTL0x36 B11001000
#elif defined LCD_LANDSCAPE_LEFT
#define MADCTL0x36 B10101000
#else
#define MADCTL0x36 B01101000
#endif   
//                  ||||||||  
//                  | B7 Page Address Order
//                   | B6 Column Address Order
//                    | B5 Page/Column Order
//                     | B4 Line Address Order
//                      | B3 RGB/BGR Order
//                       | B2 Display Data Latch Data Order
//                        | B1 Reserved Set to 0
//                         | B0 Reserved Set to 0
byte ILI948x::m_MADCTL0x36 = 0;

#ifdef SERIALIZE
bool ILI948x::_serialise = false;
#define SERIALISE_INIT(_w,_h,_s) if (ILI948x::_serialise) { Serial.print(_w);Serial.print(',');Serial.print(_h);Serial.print(',');Serial.println(_s);}
#define SERIALISE_BEGINFILL(_x,_y,_w,_h) if (ILI948x::_serialise) { Serial.print(_x);Serial.print(',');Serial.print(_y);Serial.print(',');Serial.print(_w);Serial.print(',');Serial.println(_h);}
#define SERIALISE_FILLCOLOUR(_len,_colour) if (ILI948x::_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour);}
#define SERIALISE_FILLBYTE(_len,_colour) if (ILI948x::_serialise) { Serial.print(_len);Serial.print(',');Serial.println(_colour?0xFFFF:0x0000);}
#else
#define SERIALISE_INIT(_w,_h,_s)
#define SERIALISE_BEGINFILL(_x,_y,_w,_h)
#define SERIALISE_FILLCOLOUR(_len,_colour)
#define SERIALISE_FILLBYTE(_len,_colour)
#endif

#define LCD_OR_CTRL_PORT B00100000 // Keep A5 INPUT_PULLUP (?)
/////////// UNO
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#define LCD_OR_PORTB B00011100 // Keep 10 INPUT_PULLUP and 11, 12, (SDA & SCL) HIGH
#define CTRL_PORT PORTC
#define CTRL_PIN  PINC
// Blasts into TX, RX on PORTD and Pins 10, 11, 12, 13 on PORTB, pins on PORTC, EXCEPT what is set high by LCD_OR_PORTB & LCD_OR_CTRL_PORT
#define CMD(_cmd)   { PORTD =  (_cmd); PORTB = LCD_OR_PORTB |  (_cmd); CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT;              PINC = LCD_WR_BIT; }
#define DATA(_data) { PORTD = (_data); PORTB = LCD_OR_PORTB | (_data); CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT; PINC = LCD_WR_BIT; }
#endif

/////////// MEGA
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define MEGA
//// optimised code, specific to Mega
#define CTRL_PORT PORTF
#define CTRL_PIN  PINF
// But see also ILI948x::ColourWord where this is bypassed, for speed
#define DATA_PINS(_d)   PORTE = (PORTE & 0b11000111) | (((_d) & 0b00001100) << 2) | (((_d) & 0b00100000) >> 2); \
                        PORTG = (PORTG & 0b11011111) | (((_d) & 0b00010000) << 1);                              \
                        PORTH = (PORTH & 0b10000111) | (((_d) & 0b11000000) >> 3) | ( (_d)               << 5);
                        
#define CMD(_cmd) Cmd(_cmd)
#define DATA(_data) DataByte(_data)
#endif

const byte PROGMEM ILI948x::initialisation[]  = 
{
// len, cmd,   data...
// 255, sleep MS
     3, 0xC0,  0x19, 0x1A, // Power Control 1 "VREG1OUT +ve VREG2OUT -ve"
     3, 0xC1,  0x45, 0x00, // Power Control 2 "VGH,VGL    VGH>=14V."
     2, 0xC2,  0x33,       // Power Control 3
     3, 0xC5,  0x00, 0x28, // VCOM Control "VCM_REG[7:0]. <=0X80."
     3, 0xB1,  0x90, 0x11, // Frame Rate Control "OSC Freq set. 0xA0=62HZ,0XB0 =70HZ, <=0XB0."
     2, 0xB4,  0x02,       // Display Inversion Control "2 DOT FRAME MODE,F<=70HZ."
     4, 0xB6,  0x00, 0x42, 0x3B, // Display Function Control "0 GS SS SM ISC[3:0];"
     2, 0xB7,  0x07,       // Entry Mode Set
     // Positive Gamma Control:
    16, 0xE0,  0x1F, 0x25, 0x22, 0x0B, 0x06, 0x0A, 0x4E, 0xC6, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     // Negative Gamma Control:
    16, 0xE1,  0x1F, 0x3F, 0x3F, 0x0F, 0x1F, 0x0F, 0x46, 0x49, 0x31, 0x05, 0x09, 0x03, 0x1C, 0x1A, 0x00,
     2, 0x36,  MADCTL0x36, // Memory Access Control BGR (row,col,exch)
     2, 0x3A,  0x55,       // Interface Pixel Format = 16 bits
     1, 0x11,              // Sleep OUT
   255, 120,               // delay
     //1, 0x29,              // Display ON, no, wait until we're ready
     0  // END
};

void ILI948x::Init()
{
  for(int p=2;p<10;p++)
  {
    pinMode(p,OUTPUT);
  }
  // All registers to OUTPUT, HIGH:
#ifdef MEGA
  bitSet(DDRE, 4);  // D2
  bitSet(DDRE, 5);  // D3
  bitSet(DDRG, 5);  // D4
  bitSet(DDRE, 3);  // D5
  bitSet(DDRH, 3);  // D6
  bitSet(DDRH, 4);  // D7
  bitSet(DDRH, 5);  // D8
  bitSet(DDRH, 6);  // D9  
 
  DDRF  |= (LCD_RD_PIN | LCD_WR_PIN | LCD_RS_PIN | LCD_CS_PIN | LCD_RST_PIN);
#else  
  DDRC = 0b00011111;
#endif  
  CTRL_PORT =  (LCD_RD_PIN | LCD_WR_PIN | LCD_RS_PIN | LCD_CS_PIN | LCD_RST_PIN);

  digitalWrite(LCD_RST_PIN,HIGH);
  delay(50); 
  digitalWrite(LCD_RST_PIN,LOW);
  delay(150);
  digitalWrite(LCD_RST_PIN,HIGH);
  delay(150);

  digitalWrite(LCD_CS_PIN,HIGH);
  digitalWrite(LCD_WR_PIN,HIGH);
  digitalWrite(LCD_CS_PIN,LOW);

  const byte* init = initialisation;
  do
  {
    byte len = pgm_read_byte(init++);
    if (len == 255)
    {
      delay(pgm_read_byte(init++));
    }
    else if (len)
    {
      Cmd(pgm_read_byte(init++));
      while (--len)
        DataByte(pgm_read_byte(init++));
    }
    else
    {
      break;
    }
    
  } while (true);

  m_MADCTL0x36 = MADCTL0x36;
  SERIALISE_COMMENT("*** START")
  SERIALISE_INIT(LCD_WIDTH, LCD_HEIGHT, 1);
}

void ILI948x::DisplayOn()
{
   CMD(0x29);
}

void ILI948x::Cmd(byte cmd)
{
#ifdef MEGA
  DATA_PINS(cmd);
#else  
  PORTD = cmd & B11111100; 
  PORTB = LCD_OR_PORTB | (cmd & B00000011); 
#endif  
  CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT;
  CTRL_PORT |= LCD_WR_BIT;
}

void ILI948x::DataByte(byte data)
{
#ifdef MEGA
  DATA_PINS(data);
#else  
  PORTD = data & B11111100; 
  PORTB = LCD_OR_PORTB | (data & B00000011); 
#endif  
  CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  CTRL_PORT |= LCD_WR_BIT;
}

void ILI948x::DataWord(word data)
{
  DATA((byte)(data >> 8));
  DATA((byte)(data));
}

void ILI948x::ColourByte(byte colour, unsigned long count)
{
  SERIALISE_FILLBYTE(count, colour);
  if (count)
  {
#ifdef MEGA
    DATA_PINS(colour);
#else
    PORTD = colour;
    PORTB = LCD_OR_PORTB | colour;
#endif  
    CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
    while (count--)
    {
      CTRL_PIN = LCD_WR_BIT;
      CTRL_PIN = LCD_WR_BIT;
      CTRL_PIN = LCD_WR_BIT;
      CTRL_PIN = LCD_WR_BIT;
    }
  }
}

void ILI948x::ColourWord(word colour, unsigned long count)
{
  SERIALISE_FILLCOLOUR(count, colour);
  byte hi = colour >> 8;
#ifdef MEGA
    // Go fast! The register distribution is less convenient than a Uno, there are *THREE* registers involved. 
    // Pre-compute the 6 register values:
#if 1
    // FASTEST, does NOT preserve other pins on registers E, G & H (writes 0)
    // I get ~185ms for a full screen   
    byte PORTE_lo = (((colour) & 0b00001100) << 2) | (((colour) & 0b00100000) >> 2);
    byte PORTG_lo = (((colour) & 0b00010000) << 1);
    byte PORTH_lo = (((colour) & 0b11000000) >> 3) | ( (colour)               << 5);  
    byte PORTE_hi = (((hi) & 0b00001100) << 2) | (((hi) & 0b00100000) >> 2);
    byte PORTG_hi = (((hi) & 0b00010000) << 1);
    byte PORTH_hi = (((hi) & 0b11000000) >> 3) | ( (hi)               << 5);
#else    
    // DOES preserve other pins
    // I get ~250ms for a full screen   
    byte PORTE_lo = (PORTE & 0b11000111) | (((colour) & 0b00001100) << 2) | (((colour) & 0b00100000) >> 2);
    byte PORTG_lo = (PORTG & 0b11011111) | (((colour) & 0b00010000) << 1);
    byte PORTH_lo = (PORTH & 0b10000111) | (((colour) & 0b11000000) >> 3) | ( (colour)               << 5);  
    byte PORTE_hi = (PORTE & 0b11000111) | (((hi) & 0b00001100) << 2) | (((hi) & 0b00100000) >> 2);
    byte PORTG_hi = (PORTG & 0b11011111) | (((hi) & 0b00010000) << 1);
    byte PORTH_hi = (PORTH & 0b10000111) | (((hi) & 0b11000000) >> 3) | ( (hi)               << 5);
#endif    
    CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
    while (count--)
    {
      PORTE = PORTE_hi; PORTG = PORTG_hi; PORTH = PORTH_hi;
      CTRL_PIN = LCD_WR_BIT; CTRL_PIN = LCD_WR_BIT;
      PORTE = PORTE_lo; PORTG = PORTG_lo; PORTH = PORTH_lo;
      CTRL_PIN = LCD_WR_BIT; CTRL_PIN = LCD_WR_BIT;
    }
#else
  // On a MEGA, I get ~1900ms for a full screen
  while (count--)
  {
    DATA(hi);
    DATA(colour);
  } 
#endif  
}

void ILI948x::OneWhite()
{
  SERIALISE_FILLBYTE(1, 0xFF);
#ifdef MEGA
  DATA_PINS(0xFF);
#else  
  PORTD = PORTB = 0xFF;
#endif  
  CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  CTRL_PIN = LCD_WR_BIT;
  CTRL_PIN = LCD_WR_BIT;
  CTRL_PIN = LCD_WR_BIT;
  CTRL_PIN = LCD_WR_BIT;
}

void ILI948x::OneBlack()
{
  SERIALISE_FILLBYTE(1, 0x00);
#ifdef MEGA
  DATA_PINS(0x00);
#else  
  PORTD = 0x00;
  PORTB = LCD_OR_PORTB;
#endif  
  CTRL_PORT = LCD_OR_CTRL_PORT | LCD_RST_BIT | LCD_RD_BIT | LCD_RS_BIT;
  CTRL_PIN = LCD_WR_BIT;
  CTRL_PIN = LCD_WR_BIT;
  CTRL_PIN = LCD_WR_BIT;
  CTRL_PIN = LCD_WR_BIT;
}

unsigned long ILI948x::Window(word x,word y,word w,word h)
{
  SERIALISE_BEGINFILL(x, y, w, h);

  word x2 = x + w - 1;
  word y2 = y + h - 1;
  
#ifdef LCD_FIXED_ORIGIN
  // The coordinates of the window sent to the LCD DO NOT adjust for the orientation?
  #define SWAP(_a, _b) {auto temp = _a; _a = _b; _b = temp;}
  #ifdef LCD_PORTRAIT_BOT
    // no adjustment, origin is top left
  #elif defined LCD_PORTRAIT_TOP
    x  = LCD_WIDTH - x;
    x2 = LCD_WIDTH - x2;
    SWAP(x, x2);
    y  = LCD_HEIGHT - y;
    y2 = LCD_HEIGHT - y2;
    SWAP(y, y2);    
  #elif defined LCD_LANDSCAPE_LEFT
    x  = LCD_WIDTH - x;
    x2 = LCD_WIDTH - x2;
    SWAP(x, x2);
  #else  
    y  = LCD_HEIGHT - y;
    y2 = LCD_HEIGHT - y2;
    SWAP(y, y2);
  #endif
#endif
  
  CMD(0x2A);
  DataWord(x);
  DataWord(x2);
  
  CMD(0x2B);
  DataWord(y);
  DataWord(y2);
  
  CMD(0x2C);

  unsigned long count = w;
  count *= h;
  return count;  
}

void ILI948x::SetScroll(bool left)
{
  // set direction and window
  Cmd(0x36);
  if (left)
    DataByte(m_MADCTL0x36 | B00010000); // B4=Vertical Refresh Order
  else
    DataByte(m_MADCTL0x36 & ~B00010000); // B4=Vertical Refresh Order

  // setscroll window
  Cmd(0x33);
  DataWord(0);          // TFA
  DataWord(LCD_WIDTH);  // VSA
  DataWord(0);          // BFA    
}

void ILI948x::Scroll(uint16_t cols)
{
  // actual scroll
  Cmd(0x37);
  DataWord(cols);  // VSP
}
