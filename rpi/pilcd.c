/***************************************************
  This is a library for the Adafruit 1.8" SPI display.

This library works with the Adafruit 1.8" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/358
The 1.8" TFT shield
  ----> https://www.adafruit.com/product/802
The 1.44" TFT breakout
  ----> https://www.adafruit.com/product/2088
as well as Adafruit raw 1.8" TFT display
  ----> http://www.adafruit.com/products/618

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

/*
 * this library and library functions were derived from the
 * Ada Fruit ST7735 1.8" TFT display library code for Arduino
 * the code was adapted to C, and to the unique HW setup used with 
 * my Raspberry Pi interfacing to SPI
 *
 * excellent resource here: https://warmcat.com/embedded/lcd/tft/st7735/2016/08/26/st7735-tdt-lcd-goodness.html
 *
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    <bcm2835.h>

#include    "pilcd.h"
#include    "config.h"

/* -----------------------------------------
   definitions and types
----------------------------------------- */
#define     ST7735_NOP          0x00            // LCD controller commands
#define     ST7735_SWRESET      0x01
#define     ST7735_RDDID        0x04
#define     ST7735_RDDST        0x09

#define     ST7735_SLPIN        0x10
#define     ST7735_SLPOUT       0x11
#define     ST7735_PTLON        0x12
#define     ST7735_NORON        0x13

#define     ST7735_INVOFF       0x20
#define     ST7735_INVON        0x21
#define     ST7735_DISPOFF      0x28
#define     ST7735_DISPON       0x29
#define     ST7735_CASET        0x2A
#define     ST7735_RASET        0x2B
#define     ST7735_RAMWR        0x2C
#define     ST7735_RAMRD        0x2E

#define     ST7735_PTLAR        0x30
#define     ST7735_COLMOD       0x3A
#define     ST7735_MADCTL       0x36

#define     ST7735_FRMCTR1      0xB1
#define     ST7735_FRMCTR2      0xB2
#define     ST7735_FRMCTR3      0xB3
#define     ST7735_INVCTR       0xB4
#define     ST7735_DISSET5      0xB6

#define     ST7735_PWCTR1       0xC0
#define     ST7735_PWCTR2       0xC1
#define     ST7735_PWCTR3       0xC2
#define     ST7735_PWCTR4       0xC3
#define     ST7735_PWCTR5       0xC4
#define     ST7735_VMCTR1       0xC5

#define     ST7735_RDID1        0xDA
#define     ST7735_RDID2        0xDB
#define     ST7735_RDID3        0xDC
#define     ST7735_RDID4        0xDD

#define     ST7735_PWCTR6       0xFC

#define     ST7735_GMCTRP1      0xE0
#define     ST7735_GMCTRN1      0xE1

#define     ROTATE_0            0x00            // Display rotation settings for MADCTL register
#define     ROTATE_90           0x60
#define     ROTATE_180          0xc0
#define     ROTATE_270          0xa0

#define     MADCTL_MY           0x80            // MAD display control register command bits
#define     MADCTL_MX           0x40
#define     MADCTL_MV           0x20
#define     MADCTL_ML           0x10
#define     MADCTL_RGB          0x00
#define     MADCTL_BGR          0x08
#define     MADCTL_MH           0x04

#define     DELAY               0x80
#define     ONE_MILI_SEC        260             // loop count for 1mSec (was 210)

/* -----------------------------------------
   Static functions
----------------------------------------- */
static void wait(uint16_t);
static void lcdWriteCommand(uint8_t);
static void lcdWriteData(uint8_t);
static void lcdCommandList(const uint8_t*);
static void update_row_column_addr(void);

/* -----------------------------------------
   globals
----------------------------------------- */
static int      x_start, x_end, y_start, y_end, x_loc, y_loc;
static int      _width, _height;
static int      rotation;

/* standard ascii 5x7 font
 * originally from glcdfont.c from Adafruit project
 */
static const uint8_t Font[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' ' 0
  0x3E, 0x5B, 0x4F, 0x5B, 0x3E, 0x00, // ' '
  0x3E, 0x6B, 0x4F, 0x6B, 0x3E, 0x00, // ' '
  0x1C, 0x3E, 0x7C, 0x3E, 0x1C, 0x00, // ' '
  0x18, 0x3C, 0x7E, 0x3C, 0x18, 0x00, // ' '
  0x1C, 0x57, 0x7D, 0x57, 0x1C, 0x00, // ' '
  0x1C, 0x5E, 0x7F, 0x5E, 0x1C, 0x00, // ' '
  0x00, 0x18, 0x3C, 0x18, 0x00, 0x00, // ' '
  0xFF, 0xE7, 0xC3, 0xE7, 0xFF, 0x00, // ' '
  0x00, 0x18, 0x24, 0x18, 0x00, 0x00, // ' '
  0xFF, 0xE7, 0xDB, 0xE7, 0xFF, 0x00, // ' ' 10
  0x30, 0x48, 0x3A, 0x06, 0x0E, 0x00, // ' '
  0x26, 0x29, 0x79, 0x29, 0x26, 0x00, // ' '
  0x40, 0x7F, 0x05, 0x05, 0x07, 0x00, // ' '
  0x40, 0x7F, 0x05, 0x25, 0x3F, 0x00, // ' '
  0x5A, 0x3C, 0xE7, 0x3C, 0x5A, 0x00, // ' '
  0x7F, 0x3E, 0x1C, 0x1C, 0x08, 0x00, // ' '
  0x08, 0x1C, 0x1C, 0x3E, 0x7F, 0x00, // ' '
  0x14, 0x22, 0x7F, 0x22, 0x14, 0x00, // ' '
  0x5F, 0x5F, 0x00, 0x5F, 0x5F, 0x00, // ' '
  0x06, 0x09, 0x7F, 0x01, 0x7F, 0x00, // ' ' 20
  0x00, 0x66, 0x89, 0x95, 0x6A, 0x00, // ' '
  0x60, 0x60, 0x60, 0x60, 0x60, 0x00, // ' '
  0x94, 0xA2, 0xFF, 0xA2, 0x94, 0x00, // ' '
  0x08, 0x04, 0x7E, 0x04, 0x08, 0x00, // ' '
  0x10, 0x20, 0x7E, 0x20, 0x10, 0x00, // ' '
  0x08, 0x08, 0x2A, 0x1C, 0x08, 0x00, // ' '
  0x08, 0x1C, 0x2A, 0x08, 0x08, 0x00, // ' '
  0x1E, 0x10, 0x10, 0x10, 0x10, 0x00, // ' '
  0x0C, 0x1E, 0x0C, 0x1E, 0x0C, 0x00, // ' '
  0x30, 0x38, 0x3E, 0x38, 0x30, 0x00, // ' ' 30
  0x06, 0x0E, 0x3E, 0x0E, 0x06, 0x00, // ' '
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' '
  0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, // ' '
  0x00, 0x07, 0x00, 0x07, 0x00, 0x00, // ' '
  0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00, // ' '
  0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, // ' '
  0x23, 0x13, 0x08, 0x64, 0x62, 0x00, // ' '
  0x36, 0x49, 0x56, 0x20, 0x50, 0x00, // ' '
  0x00, 0x08, 0x07, 0x03, 0x00, 0x00, // ' '
  0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, // ' ' 40
  0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, // ' '
  0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x00, // ' '
  0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, // ' '
  0x00, 0x80, 0x70, 0x30, 0x00, 0x00, // ' '
  0x08, 0x08, 0x08, 0x08, 0x08, 0x00, // ' '
  0x00, 0x00, 0x60, 0x60, 0x00, 0x00, // ' '
  0x20, 0x10, 0x08, 0x04, 0x02, 0x00, // ' '
  0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, // '0' 48
  0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, // '1' 49
  0x72, 0x49, 0x49, 0x49, 0x46, 0x00, // '2' 50
  0x21, 0x41, 0x49, 0x4D, 0x33, 0x00, // '3' 51
  0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, // '4' 52
  0x27, 0x45, 0x45, 0x45, 0x39, 0x00, // '5' 53
  0x3C, 0x4A, 0x49, 0x49, 0x31, 0x00, // '6' 54
  0x41, 0x21, 0x11, 0x09, 0x07, 0x00, // '7' 55
  0x36, 0x49, 0x49, 0x49, 0x36, 0x00, // '8' 56
  0x46, 0x49, 0x49, 0x29, 0x1E, 0x00, // '9' 57
  0x00, 0x00, 0x14, 0x00, 0x00, 0x00, // ' ' 58
  0x00, 0x40, 0x34, 0x00, 0x00, 0x00, // ' ' 59
  0x00, 0x08, 0x14, 0x22, 0x41, 0x00, // ' ' 60
  0x14, 0x14, 0x14, 0x14, 0x14, 0x00, // ' ' 61
  0x00, 0x41, 0x22, 0x14, 0x08, 0x00, // ' ' 62
  0x02, 0x01, 0x59, 0x09, 0x06, 0x00, // ' ' 63
  0x3E, 0x41, 0x5D, 0x59, 0x4E, 0x00, // ' ' 64
  0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00, // 'A' 65
  0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, // 'B' 66
  0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, // 'C' 67
  0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00, // 'D' 68
  0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, // 'E' 69
  0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, // 'F' 70
  0x3E, 0x41, 0x41, 0x51, 0x73, 0x00, // 'G' 71
  0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, // 'H' 72
  0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, // 'I' 73
  0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, // 'J' 74
  0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, // 'K' 75
  0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, // 'L' 76
  0x7F, 0x02, 0x1C, 0x02, 0x7F, 0x00, // 'M' 77
  0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, // 'N' 78
  0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, // 'O' 79
  0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, // 'P' 80
  0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, // 'Q' 81
  0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, // 'R' 82
  0x26, 0x49, 0x49, 0x49, 0x32, 0x00, // 'S' 83
  0x03, 0x01, 0x7F, 0x01, 0x03, 0x00, // 'T' 84
  0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, // 'U' 85
  0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, // 'V' 86
  0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, // 'W' 87
  0x63, 0x14, 0x08, 0x14, 0x63, 0x00, // 'X' 88
  0x03, 0x04, 0x78, 0x04, 0x03, 0x00, // 'Y' 89
  0x61, 0x59, 0x49, 0x4D, 0x43, 0x00, // 'Z' 90
  0x00, 0x7F, 0x41, 0x41, 0x41, 0x00, // ' '
  0x02, 0x04, 0x08, 0x10, 0x20, 0x00, // ' '
  0x00, 0x41, 0x41, 0x41, 0x7F, 0x00, // ' '
  0x04, 0x02, 0x01, 0x02, 0x04, 0x00, // ' '
  0x40, 0x40, 0x40, 0x40, 0x40, 0x00, // ' '
  0x00, 0x03, 0x07, 0x08, 0x00, 0x00, // ' '
  0x20, 0x54, 0x54, 0x78, 0x40, 0x00, // 'a'
  0x7F, 0x28, 0x44, 0x44, 0x38, 0x00, // 'b'
  0x38, 0x44, 0x44, 0x44, 0x28, 0x00, // 'c'
  0x38, 0x44, 0x44, 0x28, 0x7F, 0x00, // 'd' 100
  0x38, 0x54, 0x54, 0x54, 0x18, 0x00, // 'e'
  0x00, 0x08, 0x7E, 0x09, 0x02, 0x00, // 'f'
  0x18, 0xA4, 0xA4, 0x9C, 0x78, 0x00, // 'g'
  0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, // 'h'
  0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, // 'i'
  0x20, 0x40, 0x40, 0x3D, 0x00, 0x00, // 'j'
  0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, // 'k'
  0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, // 'l'
  0x7C, 0x04, 0x78, 0x04, 0x78, 0x00, // 'm'
  0x7C, 0x08, 0x04, 0x04, 0x78, 0x00, // 'n' 110
  0x38, 0x44, 0x44, 0x44, 0x38, 0x00, // 'o'
  0xFC, 0x18, 0x24, 0x24, 0x18, 0x00, // 'p'
  0x18, 0x24, 0x24, 0x18, 0xFC, 0x00, // 'q'
  0x7C, 0x08, 0x04, 0x04, 0x08, 0x00, // 'r'
  0x48, 0x54, 0x54, 0x54, 0x24, 0x00, // 's'
  0x04, 0x04, 0x3F, 0x44, 0x24, 0x00, // 't'
  0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00, // 'u'
  0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, // 'v'
  0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00, // 'w'
  0x44, 0x28, 0x10, 0x28, 0x44, 0x00, // 'x' 120
  0x4C, 0x90, 0x90, 0x90, 0x7C, 0x00, // 'y'
  0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, // 'z'
  0x00, 0x08, 0x36, 0x41, 0x00, 0x00, // '{'
  0x00, 0x00, 0x77, 0x00, 0x00, 0x00, // '|'
  0x00, 0x41, 0x36, 0x08, 0x00, 0x00, // '}'
  0x02, 0x01, 0x02, 0x04, 0x02, 0x00, // '~'
  0x3C, 0x26, 0x23, 0x26, 0x3C, 0x00, // ' '
  0x1E, 0xA1, 0xA1, 0x61, 0x12, 0x00, // ' '
  0x3A, 0x40, 0x40, 0x20, 0x7A, 0x00, // ' '
  0x38, 0x54, 0x54, 0x55, 0x59, 0x00, // ' ' 130
  0x21, 0x55, 0x55, 0x79, 0x41, 0x00, // ' '
  0x21, 0x54, 0x54, 0x78, 0x41, 0x00, // ' '
  0x21, 0x55, 0x54, 0x78, 0x40, 0x00, // ' '
  0x20, 0x54, 0x55, 0x79, 0x40, 0x00, // ' '
  0x0C, 0x1E, 0x52, 0x72, 0x12, 0x00, // ' '
  0x39, 0x55, 0x55, 0x55, 0x59, 0x00, // ' '
  0x39, 0x54, 0x54, 0x54, 0x59, 0x00, // ' '
  0x39, 0x55, 0x54, 0x54, 0x58, 0x00, // ' '
  0x00, 0x00, 0x45, 0x7C, 0x41, 0x00, // ' '
  0x00, 0x02, 0x45, 0x7D, 0x42, 0x00, // ' ' 140
  0x00, 0x01, 0x45, 0x7C, 0x40, 0x00, // ' '
  0xF0, 0x29, 0x24, 0x29, 0xF0, 0x00, // ' '
  0xF0, 0x28, 0x25, 0x28, 0xF0, 0x00, // ' '
  0x7C, 0x54, 0x55, 0x45, 0x00, 0x00, // ' '
  0x20, 0x54, 0x54, 0x7C, 0x54, 0x00, // ' '
  0x7C, 0x0A, 0x09, 0x7F, 0x49, 0x00, // ' '
  0x32, 0x49, 0x49, 0x49, 0x32, 0x00, // ' '
  0x32, 0x48, 0x48, 0x48, 0x32, 0x00, // ' '
  0x32, 0x4A, 0x48, 0x48, 0x30, 0x00, // ' '
  0x3A, 0x41, 0x41, 0x21, 0x7A, 0x00, // ' ' 150
  0x3A, 0x42, 0x40, 0x20, 0x78, 0x00, // ' '
  0x00, 0x9D, 0xA0, 0xA0, 0x7D, 0x00, // ' '
  0x39, 0x44, 0x44, 0x44, 0x39, 0x00, // ' '
  0x3D, 0x40, 0x40, 0x40, 0x3D, 0x00, // ' '
  0x3C, 0x24, 0xFF, 0x24, 0x24, 0x00, // ' '
  0x48, 0x7E, 0x49, 0x43, 0x66, 0x00, // ' '
  0x2B, 0x2F, 0xFC, 0x2F, 0x2B, 0x00, // ' '
  0xFF, 0x09, 0x29, 0xF6, 0x20, 0x00, // ' '
  0xC0, 0x88, 0x7E, 0x09, 0x03, 0x00, // ' '
  0x20, 0x54, 0x54, 0x79, 0x41, 0x00, // ' ' 160
  0x00, 0x00, 0x44, 0x7D, 0x41, 0x00, // ' '
  0x30, 0x48, 0x48, 0x4A, 0x32, 0x00, // ' '
  0x38, 0x40, 0x40, 0x22, 0x7A, 0x00, // ' '
  0x00, 0x7A, 0x0A, 0x0A, 0x72, 0x00, // ' '
  0x7D, 0x0D, 0x19, 0x31, 0x7D, 0x00, // ' '
  0x26, 0x29, 0x29, 0x2F, 0x28, 0x00, // ' '
  0x26, 0x29, 0x29, 0x29, 0x26, 0x00, // ' '
  0x30, 0x48, 0x4D, 0x40, 0x20, 0x00, // ' '
  0x38, 0x08, 0x08, 0x08, 0x08, 0x00, // ' '
  0x08, 0x08, 0x08, 0x08, 0x38, 0x00, // ' ' 170
  0x2F, 0x10, 0xC8, 0xAC, 0xBA, 0x00, // ' '
  0x2F, 0x10, 0x28, 0x34, 0xFA, 0x00, // ' '
  0x00, 0x00, 0x7B, 0x00, 0x00, 0x00, // ' '
  0x08, 0x14, 0x2A, 0x14, 0x22, 0x00, // ' '
  0x22, 0x14, 0x2A, 0x14, 0x08, 0x00, // ' '
  0xAA, 0x00, 0x55, 0x00, 0xAA, 0x00, // ' '
  0x55, 0x00, 0xAA, 0x00, 0x55, 0x00, // ' '
  0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, // ' '
  0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, // ' '
  0x10, 0x10, 0x10, 0xFF, 0x00, 0x00, // ' ' 180
  0x14, 0x14, 0x14, 0xFF, 0x00, 0x00, // ' '
  0x10, 0x10, 0xFF, 0x00, 0xFF, 0x00, // ' '
  0x10, 0x10, 0xF0, 0x10, 0xF0, 0x00, // ' '
  0x14, 0x14, 0x14, 0xFC, 0x00, 0x00, // ' '
  0x14, 0x14, 0xF7, 0x00, 0xFF, 0x00, // ' '
  0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, // ' '
  0x14, 0x14, 0xF4, 0x04, 0xFC, 0x00, // ' '
  0x14, 0x14, 0x17, 0x10, 0x1F, 0x00, // ' '
  0x10, 0x10, 0x1F, 0x10, 0x1F, 0x00, // ' '
  0x14, 0x14, 0x14, 0x1F, 0x00, 0x00, // ' ' 190
  0x10, 0x10, 0x10, 0xF0, 0x00, 0x00, // ' '
  0x00, 0x00, 0x00, 0x1F, 0x10, 0x10, // ' '
  0x10, 0x10, 0x10, 0x1F, 0x10, 0x10, // ' '
  0x10, 0x10, 0x10, 0xF0, 0x10, 0x10, // ' '
  0x00, 0x00, 0x00, 0xFF, 0x10, 0x10, // ' '
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, // ' '
  0x10, 0x10, 0x10, 0xFF, 0x10, 0x10, // ' '
  0x00, 0x00, 0x00, 0xFF, 0x14, 0x14, // ' '
  0x00, 0x00, 0xFF, 0x00, 0xFF, 0x10, // ' '
  0x00, 0x00, 0x1F, 0x10, 0x17, 0x14, // ' ' 200
  0x00, 0x00, 0xFC, 0x04, 0xF4, 0x14, // ' '
  0x14, 0x14, 0x17, 0x10, 0x17, 0x14, // ' '
  0x14, 0x14, 0xF4, 0x04, 0xF4, 0x14, // ' '
  0x00, 0x00, 0xFF, 0x00, 0xF7, 0x14, // ' '
  0x14, 0x14, 0x14, 0x14, 0x14, 0x14, // ' '
  0x14, 0x14, 0xF7, 0x00, 0xF7, 0x14, // ' '
  0x14, 0x14, 0x14, 0x17, 0x14, 0x14, // ' '
  0x10, 0x10, 0x1F, 0x10, 0x1F, 0x14, // ' '
  0x14, 0x14, 0x14, 0xF4, 0x14, 0x14, // ' '
  0x10, 0x10, 0xF0, 0x10, 0xF0, 0x10, // ' ' 210
  0x00, 0x00, 0x1F, 0x10, 0x1F, 0x10, // ' '
  0x00, 0x00, 0x00, 0x1F, 0x14, 0x14, // ' '
  0x00, 0x00, 0x00, 0xFC, 0x14, 0x14, // ' '
  0x00, 0x00, 0xF0, 0x10, 0xF0, 0x10, // ' '
  0x10, 0x10, 0xFF, 0x10, 0xFF, 0x10, // ' '
  0x14, 0x14, 0x14, 0xFF, 0x14, 0x14, // ' '
  0x10, 0x10, 0x10, 0x1F, 0x00, 0x00, // ' '
  0x00, 0x00, 0x00, 0xF0, 0x10, 0x10, // ' '
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xff, // ' '
  0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xf0, // ' ' 220
  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, // ' '
  0x00, 0x00, 0x00, 0xFF, 0xFF, 0xff, // ' '
  0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0f, // ' '
  0x38, 0x44, 0x44, 0x38, 0x44, 0x00, // ' '
  0x7C, 0x2A, 0x2A, 0x3E, 0x14, 0x00, // ' '
  0x7E, 0x02, 0x02, 0x06, 0x06, 0x00, // ' '
  0x02, 0x7E, 0x02, 0x7E, 0x02, 0x00, // ' '
  0x63, 0x55, 0x49, 0x41, 0x63, 0x00, // ' '
  0x38, 0x44, 0x44, 0x3C, 0x04, 0x00, // ' '
  0x40, 0x7E, 0x20, 0x1E, 0x20, 0x00, // ' ' 230
  0x06, 0x02, 0x7E, 0x02, 0x02, 0x00, // ' '
  0x99, 0xA5, 0xE7, 0xA5, 0x99, 0x00, // ' '
  0x1C, 0x2A, 0x49, 0x2A, 0x1C, 0x00, // ' '
  0x4C, 0x72, 0x01, 0x72, 0x4C, 0x00, // ' '
  0x30, 0x4A, 0x4D, 0x4D, 0x30, 0x00, // ' '
  0x30, 0x48, 0x78, 0x48, 0x30, 0x00, // ' '
  0xBC, 0x62, 0x5A, 0x46, 0x3D, 0x00, // ' '
  0x3E, 0x49, 0x49, 0x49, 0x00, 0x00, // ' '
  0x7E, 0x01, 0x01, 0x01, 0x7E, 0x00, // ' '
  0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x00, // ' ' 240
  0x44, 0x44, 0x5F, 0x44, 0x44, 0x00, // ' '
  0x40, 0x51, 0x4A, 0x44, 0x40, 0x00, // ' '
  0x40, 0x44, 0x4A, 0x51, 0x40, 0x00, // ' '
  0x00, 0x00, 0xFF, 0x01, 0x03, 0x00, // ' '
  0xE0, 0x80, 0xFF, 0x00, 0x00, 0x00, // ' '
  0x08, 0x08, 0x6B, 0x6B, 0x08, 0x00, // ' '
  0x36, 0x12, 0x36, 0x24, 0x36, 0x00, // ' '
  0x06, 0x0F, 0x09, 0x0F, 0x06, 0x00, // ' '
  0x00, 0x00, 0x18, 0x18, 0x00, 0x00, // ' '
  0x00, 0x00, 0x10, 0x10, 0x00, 0x00, // ' ' 250
  0x30, 0x40, 0xFF, 0x01, 0x01, 0x00, // ' '
  0x00, 0x1F, 0x01, 0x01, 0x1E, 0x00, // ' '
  0x00, 0x19, 0x1D, 0x17, 0x12, 0x00, // ' '
  0x00, 0x3C, 0x3C, 0x3C, 0x3C, 0x00, // ' '
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' '
};

// rather than lcdWriteCommand() and lcdWriteData() calls, screen
// initialization commands and arguments are organized in a table.
// table is read, parsed and issues by lcdCommandList()
static const uint8_t
initSeq[] =
    {                                       // consolidated initialization sequence
        21,                                 // 21 commands in list:
        ST7735_SWRESET,   DELAY,            //  1: Software reset, 0 args, w/delay
          150,                              //     150 ms delay
        ST7735_SLPOUT ,   DELAY,            //  2: Out of sleep mode, 0 args, w/delay
          150,                              //     150mSec delay per spec pg.94             *** was 500 ms delay value 255)
        ST7735_FRMCTR1, 3      ,            //  3: Frame rate ctrl - normal mode, 3 args:
          0x01, 0x2C, 0x2D,                 //     Rate = fosc/((1x2+40) * (LINE+2C+2D+2))  *** several opinions here, like  0x00, 0x06, 0x03,
        ST7735_FRMCTR2, 3      ,            //  4: Frame rate control - idle mode, 3 args:
          0x01, 0x2C, 0x2D,                 //     Rate = fosc/(1x2+40) * (LINE+2C+2D)      *** same as above
        ST7735_FRMCTR3, 6      ,            //  5: Frame rate ctrl - partial mode, 6 args:
          0x01, 0x2C, 0x2D,                 //     Dot inversion mode                       *** same as above
          0x01, 0x2C, 0x2D,                 //     Line inversion mode
        ST7735_INVCTR , 1      ,            //  6: Display inversion ctrl, 1 arg, no delay:
          0x07,                             //     No inversion                             *** set to post-reset default value
        ST7735_PWCTR1 , 3      ,            //  7: Power control, 3 args, no delay:
          0xA2,                             //     AVDD = 5v, VRHP = 4.6v
          0x02,                             //     VRHN = -4.6V
          0x84,                             //     AUTO mode
        ST7735_PWCTR2 , 1      ,            //  8: Power control, 1 arg, no delay:
          0xC5,                             //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
        ST7735_PWCTR3 , 2      ,            //  9: Power control, 2 args, no delay:
          0x0A,                             //     Opamp current small
          0x00,                             //     Boost frequency
        ST7735_PWCTR4 , 2      ,            // 10: Power control, 2 args, no delay:
          0x8A,                             //     BCLK/2, Opamp current small & Medium low
          0x2A,
        ST7735_PWCTR5 , 2      ,            // 11: Power control, 2 args, no delay:
          0x8A, 0xEE,
        ST7735_VMCTR1 , 1      ,            // 12: Power control, 1 arg, no delay:
          0x0E,
        ST7735_INVOFF , 0      ,            // 13: Don't invert display, no args, no delay
        ST7735_MADCTL , 1      ,            // 14: Memory access control (directions), 1 arg:
          ROTATE_0,                         //     Normal rotation, RGB color order
        ST7735_COLMOD , 1      ,            // 15: set color mode, 1 arg, no delay:
          0x05,                             //     16-bit color
        ST7735_CASET  , 4      ,            // 16: Column addr set, 4 args, no delay:
          0x00, 0x00,                       //     XSTART = 0
          0x00, 0x7F,                       //     XEND   = 127
        ST7735_RASET  , 4      ,            // 17: Row addr set, 4 args, no delay:
          0x00, 0x00,                       //     YSTART = 0
          0x00, 0x9F,                       //     YEND   = 159
        ST7735_GMCTRP1, 16      ,           // 18: Gamma (‘+’polarity) Correction Characteristics Setting, 16 args, no delay:
          0x02, 0x1c, 0x07, 0x12,
          0x37, 0x32, 0x29, 0x2d,
          0x29, 0x25, 0x2B, 0x39,
          0x00, 0x01, 0x03, 0x10,
        ST7735_GMCTRN1, 16      ,           // 19: Gamma ‘-’polarity Correction Characteristics Setting, 16 args, no delay:
          0x03, 0x1d, 0x07, 0x06,
          0x2E, 0x2C, 0x29, 0x2D,
          0x2E, 0x2E, 0x37, 0x3F,
          0x00, 0x00, 0x02, 0x10,
        ST7735_NORON  ,    DELAY,           // 20: Normal display on, no args, w/delay
          10,                               //     10 ms delay
        ST7735_DISPON ,    DELAY,           // 21: Main screen turn on, no args w/delay
          100
    };

/* -----------------------------------------
   driver functions
----------------------------------------- */

/*------------------------------------------------
 * wait()
 *
 *  Delay based on BCM2835 timer
 *
 */
static void wait(uint16_t miliSec)
{
    if ( miliSec < 1 )
        miliSec = 1;
    
    bcm2835_delay(miliSec);
}

/*------------------------------------------------
 * lcdWriteCommand()
 *
 *  write a command byte to the ST7735 LCD
 *
 */
static void lcdWriteCommand(uint8_t byte)
{
    // select CMD mode and write byte
    bcm2835_gpio_write(LCD_DATA_CMD, LOW);
    //bcm2835_spi_write(byte);
    bcm2835_spi_transfer(byte);
}

/*------------------------------------------------
 * lcdWriteData()
 *
 *  write a data byte to the ST7735 LCD
 *
 */
static void lcdWriteData(uint8_t byte)
{
    // select DATA mode and write byte
    bcm2835_gpio_write(LCD_DATA_CMD, HIGH);
    //bcm2835_spi_write(byte);
    bcm2835_spi_transfer(byte);
}

/*------------------------------------------------
 * lcdCommandList()
 *
 *  reads and issues a series of LCD commands groupd
 *  inside the initialized data tables
 *
 */
static void lcdCommandList(const uint8_t *addr)
{
    int     numCommands, numArgs;
    int     ms;

    numCommands = *(addr++);                    // Number of commands to follow
    while ( numCommands-- )                     // For each command...
    {
        lcdWriteCommand(*(addr++));             // Read, issue command
        numArgs  = *(addr++);                   // Number of args to follow
        ms       = numArgs & DELAY;             // If hibit set, delay follows args
        numArgs &= ~DELAY;                      // Mask out delay bit
        while ( numArgs-- )                     // For each argument...
        {
            lcdWriteData(*(addr++));            // Read, issue argument
        }

        if ( ms )
        {
            ms = *(addr++);                     // Read post-command delay time (ms)
            if ( ms == 255 )
                ms = 500;                       // If 255, delay for 500 ms
            wait(ms);
        }
    }
}

/*------------------------------------------------
 * update_row_column_addr()
 *
 *  Update row (y_loc) and column (x_loc) addresses
 *
 */
static void update_row_column_addr(void)
{
    x_loc++;
    if ( x_loc > x_end || x_loc == _width )
    {
        x_loc = x_start;
        y_loc++;
        if ( y_loc > y_end || y_loc == _height )
            y_loc = y_end;
    }
}

/*------------------------------------------------
 * lcdInit()
 *
 *  initialization of LCD screen
 *
 */
void lcdInit(void)
{
    x_start  = 0;
    x_end    = 0;
    y_start  = 0;
    y_end    = 0;
    x_loc    = 0;
    y_loc    = 0;
    _width   = 0;
    _height  = 0;
    rotation = 0;

    bcm2835_gpio_fsel(LCD_DATA_CMD, HIGH);        // setup CMD/DATA GPIO line
    bcm2835_gpio_write(LCD_DATA_CMD, HIGH);
    
    lcdCommandList(initSeq);                    // initialization commands to the display
    lcdSetRotation(ROTATE_0);                   // set display rotation and size defaults
}

/*------------------------------------------------
 * lcdOn()
 *
 *  turn LCD on
 *
 */
void lcdOn(void)
{
    lcdWriteCommand(ST7735_DISPON);
    wait(100);
}

/*------------------------------------------------
 * lcdOff()
 *
 *  turn LCD off
 *
 */
void lcdOff(void)
{
    lcdWriteCommand(ST7735_DISPOFF);
    wait(100);
}

/*------------------------------------------------
 * lcdHeight()
 *
 *  LCD height in pixels
 *
 */
int lcdHeight(void)
{
    return _height;
}

/*------------------------------------------------
 * lcdWidth()
 *
 *  return LCD width in pixels
 *
 */
int lcdWidth(void)
{
    return _width;
}

/*------------------------------------------------
 * lcdSetRotation()
 *
 *  screen rotation
 *
 */
void lcdSetRotation(uint8_t mode)
{
    uint8_t     ctrlByte = 0x00;

    if ( mode > 3 )
        return;

    switch ( mode )
    {
        case 0:
            ctrlByte = ROTATE_0;
            _width  = ST7735_TFTWIDTH;
            _height = ST7735_TFTHEIGHT;
            break;

        case 1:
            ctrlByte = ROTATE_90;
            _width = ST7735_TFTHEIGHT;
            _height = ST7735_TFTWIDTH;
             break;

        case 2:
            ctrlByte = ROTATE_180;
            _width  = ST7735_TFTWIDTH;
            _height = ST7735_TFTHEIGHT;
            break;

        case 3:
            ctrlByte = ROTATE_270;
            _width = ST7735_TFTHEIGHT;
            _height = ST7735_TFTWIDTH;
            break;
    }

    x_start  = 0;
    x_end    = _width - 1;
    y_start  = 0;
    y_end    = _height - 1;
    x_loc    = x_start;
    y_loc    = y_start;

    lcdWriteCommand(ST7735_MADCTL);
    lcdWriteData(ctrlByte);
}

/*------------------------------------------------
 * lcdInvertDisplay()
 *
 *  inverse screen colors
 *
 */
void lcdInvertDisplay(int i)
{
    lcdWriteCommand(i ? ST7735_INVON : ST7735_INVOFF);
}

/*------------------------------------------------
 * lcdColor565()
 *
 *  Pass 8-bit (each) R,G,B, get back 16-bit packed color
 *
 */
uint16_t lcdColor565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/*------------------------------------------------
 * lcdSetAddrWindow()
 *
 *  set LCD window size in pixels from top left to bottom right
 *  and setup for write to LCD RAM frame buffer
 *  any subsequent write commands will go to RAN and be frawn on the display
 *
 */
void lcdSetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    x_start = x0;
    x_end   = x1;
    y_start = y0;
    y_end   = y1;
    x_loc   = x_start;
    y_loc   = y_start;

    lcdWriteCommand(ST7735_CASET);  // Column addr set
    lcdWriteData(0x00);
    lcdWriteData(x0);               // XSTART
    lcdWriteData(0x00);
    lcdWriteData(x1);               // XEND

    lcdWriteCommand(ST7735_RASET);  // Row addr set
    lcdWriteData(0x00);
    lcdWriteData(y0);               // YSTART
    lcdWriteData(0x00);
    lcdWriteData(y1);               // YEND

    lcdWriteCommand(ST7735_RAMWR);  // write to RAM
}

/*------------------------------------------------
 * lcdFillScreen()
 *
 *  fill screen with solid color
 *
 */
void lcdFillScreen(uint16_t color)
{
    lcdFillRect(0, 0,  _width, _height, color);
}

/*------------------------------------------------
 * lcdDrawPixel()
 *
 *  drow a pixes in coordinate (x,y) with color
 *
 */
void lcdDrawPixel(int x, int y, uint16_t color)
{
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
        return;

    lcdSetAddrWindow(x,y,x+1,y+1);
  
    lcdWriteData((uint8_t) (color >> 8));
    lcdWriteData((uint8_t) color);
}

/*------------------------------------------------
 * lcdDrawFastVLine()
 *
 *  draw vertical line from x,y length h and color
 *
 */
void lcdDrawFastVLine(int x, int y, int h, uint16_t color)
{
    uint8_t     hi, lo;

    // Rudimentary clipping
    if ((x >= _width) || (y >= _height))
        return;

    if ((y+h-1) >= _height)
        h = _height-y;

    lcdSetAddrWindow(x, y, x, y+h-1);

    hi = (uint8_t) (color >> 8);
    lo = (uint8_t) color;
    
    while (h--)
    {
        lcdWriteData(hi);
        lcdWriteData(lo);
    }
}

/*------------------------------------------------
 * lcdDrawFastHLine()
 *
 *  draw horizontal line from x,y with length w and color
 *
 */
void lcdDrawFastHLine(int x, int y, int w, uint16_t color)
{
    uint8_t     hi, lo;

    // Rudimentary clipping
    if ((x >= _width) || (y >= _height))
        return;

    if ((x+w-1) >= _width)
        w = _width-x;

    lcdSetAddrWindow(x, y, x+w-1, y);

    hi = (uint8_t) (color >> 8);
    lo = (uint8_t) color;

    while (w--)
    {
        lcdWriteData(hi);
        lcdWriteData(lo);
    }
}

/*------------------------------------------------
 * lcdFillRect()
 *
 *  draw a filled rectangle with solid color
 *
 */
void lcdFillRect(int x, int y, int w, int h, uint16_t color)
{
    uint8_t     hi, lo;

    // rudimentary clipping (drawChar w/big text requires this)
    if ((x >= _width) || (y >= _height))
        return;

    if ((x + w - 1) >= _width)
        w = _width  - x;

    if ((y + h - 1) >= _height)
        h = _height - y;

    lcdSetAddrWindow(x, y, x+w-1, y+h-1);

    hi = (uint8_t) (color >> 8);
    lo = (uint8_t) color;
    
    for (y=h; y>0; y--)
    {
        for (x=w; x>0; x--)
        {
            lcdWriteData(hi);
            lcdWriteData(lo);
        }
    }
}

/*------------------------------------------------
 * lcdFrameBufferInit()
 *
 *  initialize a frame buffer with a color
 *
 */
uint8_t* lcdFrameBufferInit(uint16_t color)
{
    uint8_t*  buffer;
    uint16_t  size;
    uint16_t  i;

    size = _width * _height * sizeof(uint16_t);         // calculate buffer size

    if ( (buffer = (uint8_t*) malloc(size)) == 0 )      // allocate memory and abort if cannot
        return 0;

    for ( i = 0; i < size; i += 2)                      // initialize buffer with color
    {
        buffer[i] = (uint8_t) (color >> 8);             // swap bytes in buffer
        buffer[i+1] = (uint8_t) color;                  // so that writes yield correct order
    }

    return buffer;
}

/*------------------------------------------------
 * lcdFrameBufferFree()
 *
 *  release memory reserved for the frame buffer
 *
 */
void lcdFrameBufferFree(uint8_t* frameBufferPointer)
{
    free((void*) frameBufferPointer);                       // free buffer memory
}

/*------------------------------------------------
 * lcdFrameBufferPush()
 *
 *  tranfer frame buffer to LCD
 *
 */
void lcdFrameBufferPush(uint8_t* frameBufferPointer)
{
    uint16_t  size;

    size = _width * _height * sizeof(uint16_t);             // calculate frame buffer size
    lcdSetAddrWindow(0, 0, _width-1, _height-1);            // prepare display area

    // select DATA mode and write byte
    bcm2835_gpio_write(LCD_DATA_CMD, HIGH);
    bcm2835_spi_writenb(frameBufferPointer, size);
}

/*------------------------------------------------
 * lcdFrameBufferColor()
 *
 *  initialize an existing (allocated) frame buffer with a color
 *
 */
void lcdFrameBufferColor(uint8_t* frameBufferPointer, uint16_t color)
{
    uint16_t    size;
    uint16_t    i;
    
    size = _width * _height * sizeof(uint16_t);         // calculate buffer size

    for ( i = 0; i < size; i += 2)                      // initialize buffer with color
    {
        frameBufferPointer[i] = (uint8_t) (color >> 8); // swap bytes in buffer
        frameBufferPointer[i+1] = (uint8_t) color;      // so that writes yield correct order
    }
}

/*------------------------------------------------
 * lcdFrameBufferScroll()
 *
 *  scroll frame buffer by +/- pixels and
 *  fill new lines with color
 *
 */
void lcdFrameBufferScroll(int pixels, uint16_t color)
{
}

/*------------------------------------------------
 * lcdPushColor()
 *
 *  send color pixel to LCD or an allocated frame buffer
 *  must run after lcdSetAddrWindow()
 *
 * param:  frameBuff  pointer to allocated frame buffer, if NULL the function writes direct to screen
 *         color      16-bit color
 * return: none
 *
 */
void lcdPushColor(uint8_t* frameBuff, uint16_t color)
{
    int     buff_indx;

    // if a valid buffer address is passed, then write to the buffer
    if ( frameBuff )
    {
        buff_indx = 2*(x_loc + y_loc*_width);
        frameBuff[buff_indx] = (uint8_t) (color >> 8);
        frameBuff[buff_indx+1] = (uint8_t) color;
    }

    // otherwise write pixel direct to LCD screen
    else
    {
        lcdWriteData((uint8_t) (color >> 8));
        lcdWriteData((uint8_t) color);
    }

    // update row and column address variable
    update_row_column_addr();
}

/*------------------------------------------------
 * lcdDrawChar()
 *
 * Similar to the function from Adafruit_GFX.c but adapted for this processor.
 * This function uses one call to lcdSetAddrWindow(), which allows it to
 * run at least twice as fast.
 *
 * param:  frameBuff   pointer to allocated frame buffer, if NULL the function writes direct to screen
 *         x           horizontal position of the top left corner of the character, columns from the left edge
 *         y           vertical position of the top left corner of the character, rows from the top edge
 *         c           character to be printed
 *         textColor   16-bit color of the character
 *         bgColor     16-bit color of the background
 *         scale       number of pixels per character pixel (e.g. size==2 prints each pixel of font as 2x2 square)
 *         transparent '0' color in the background, '1' no background when writing to frame buffer
 * return: none
 *
 */
void lcdDrawChar(uint8_t* frameBuff, uint16_t x, uint16_t y, char c, uint16_t textColor, uint16_t bgColor, int scale, int transparent)
{
    uint8_t   line;             // horizontal row of pixels of character
    uint16_t  col, row, i, j;   // loop indices

    // do some range checking of 'x' an 'y' coordinates
    if (((x + FONT_PIX_WIDE*scale - 1) >= _width)  ||
        ((y + FONT_PIX_HIGH*scale - 1) >= _height))
    {
        return;
    }

    lcdSetAddrWindow(x, y, x+FONT_PIX_WIDE*scale-1, y+FONT_PIX_HIGH*scale-1);

    // print character rows starting at the top row
    // print the columns, starting on the left
    line = 0x01;
    for ( row = 0; row < FONT_PIX_HIGH; row++ )
    {
        for (i = 0; i < scale; i++ )
        {
            for ( col = 0; col < FONT_PIX_WIDE; col++ )
            {
                for ( j = 0; j < scale; j++ )
                {
                    // Bit is set in Font, print pixel(s) in text color
                    if ( Font[(c*FONT_PIX_WIDE+col)] & line )
                        lcdPushColor(frameBuff, textColor);

                    // Bit is cleared in Font
                    else
                    {
                        // Always paint background on LCD, or if background is not transparent
                        if ( frameBuff == NULL || !transparent )
                            lcdPushColor(frameBuff, bgColor);

                        // Pixel is transparent and sent only to buffer
                        // so only advance row/column positions
                        else
                            update_row_column_addr();
                    }
                }
            }
        }

        // move up to the next row
        line = line << 1;
    }
}
