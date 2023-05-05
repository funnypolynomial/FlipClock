#pragma once

/*
 * UNO SCHEMATIC 
 * Blank stackable shield (eg https://www.jaycar.co.nz/arduino-compatible-prototyping-board-shield/p/XC4482)
 * Sits between Uno and LCD, shown from the LCD side (top)
 * Allows access to the pins not used by the LCD
 *               +----------Shield----------+
 *               |                    SCL[ ]|
 *               |                    SDA[ ]|
 *               |                   AREF[ ]|
 *               |[ ]IOREF            GND[*]|                  +-DS3231-RTC-Breakout-+
 *               |[ ]IOREF         led 13[ ]|                  |                 RST-+---[Reset]
 *    [Reset]----+[*]RST               12[*]+------------------+-SCL             Vin-+---[5V]
 *               |[ ]3V3               11[*]+------------------+-SDA             GND-+---[GND]
 *               |[*]5V                10[*]+--------------+   +---------------------+
 *               |[ ]GND                9[X]|---LCD:D1     |
 *               |[ ]GND                8[X]|-------D0     +------[Button:SET]
 *               |[ ]Vin                7[X]|-------D7
 *               |[ ]RST                6[X]|-------D6
 *      LCD:RD---|[X]A0                 5[X]|-------D5
 *          WR---|[X]A1                 4[X]|-------D4
 *          RS---|[X]A2                 3[X]|-------D3
 *          CS---|[X]A3                 2[X]|-------D2
 *          RST--|[X]A4              TX 1[ ]|
 *             +-+[*]A5              RX 0[ ]|
 *             | |                  --------+
 *             |  +----------------/         
 *             | 
 *             +--------------------------------------------------[Button:ADJ]
 *  
 *                             _ (push-button, normally open)
 *        Button is [GND]------ -------[PIN]
 *  
 *  Notes:
 *  RTC's 5V, GND and RST are connected to Shield equivalents.
 *  Pins 11 & 12 provide a *SOFTWARE* I2C link to the RTC.
 *  Pins 10 & A5 are INPUT_PULLUP, connected to push-button, normally open, connected to GND when closed.
 */

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
// See the comment in ILI948x::ColourWord about available pins
#define PIN_BTN_1_SET 25
#define PIN_BTN_2_ADJ 23
#define PIN_SDA       27
#define PIN_SCL       29
#else
// note that the LCD code knows about these and preserves the pins as INPUT_PULLUP
#define PIN_BTN_1_SET 10
#define PIN_BTN_2_ADJ A5

#define PIN_SDA       11
#define PIN_SCL       12
#endif
