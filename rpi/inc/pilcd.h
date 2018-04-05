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
 */

#include    <stdint.h>

#ifndef     __pilcd_h__
#define     __pilcd_h__

// Color definitions
#define	    ST7735_BLACK        0x0000
#define	    ST7735_BLUE         0x001F
#define	    ST7735_RED          0xF800
#define	    ST7735_GREEN        0x07E0
#define     ST7735_CYAN         0x07FF
#define     ST7735_MAGENTA      0xF81F
#define     ST7735_YELLOW       0xFFE0
#define     ST7735_WHITE        0xFFFF

#define     ST7735_TFTWIDTH     128             // 1.8" LCD pixes geometry
#define     ST7735_TFTHEIGHT    160

#define     FONT_PIX_WIDE       6
#define     FONT_PIX_HIGH       8

/* -----------------------------------------
   Function prototypes
----------------------------------------- */

/*------------------------------------------------
 *  General display functions
 *
 */
void        lcdInit(void);                                          // initialize LCD
void        lcdOn(void);                                            // turn LCD on
void        lcdOff(void);                                           // turn LCD off
int         lcdHeight(void);                                        // return display pixel height
int         lcdWidth(void);                                         // return display pixel width
void        lcdSetRotation(uint8_t);                                // screen rotation
void        lcdInvertDisplay(int);                                  // invert display
uint16_t    lcdColor565(uint8_t, uint8_t, uint8_t);                 // Pass 8-bit (each) R,G,B, get back 16-bit packed color

/*------------------------------------------------
 *  Direct access display functions
 *  TODO change these to direct or frame buffer functions
 */
void        lcdFillRect(int, int, int, int, uint16_t);              // color filled rectangle

/*------------------------------------------------
 *  Frame buffer display functions
 *  (using a fixed 128x160 pixel frame buffer)
 *
 */
uint8_t*    lcdFrameBufferInit(uint16_t);                           // allocate and initialize a frame buffer with a color
void        lcdFrameBufferFree(uint8_t*);                           // release memory reserved for the frame buffer
void        lcdFrameBufferPush(uint8_t*);                           // transfer frame buffer to LCD
void        lcdFrameBufferColor(uint8_t*, uint16_t);                // initialize an existing (allocated) frame buffer with a color
void        lcdFrameBufferScroll(int, uint16_t);                    // scroll frame buffer by +/- pixels and fill new lines with color

/*------------------------------------------------
 *  Graphics functions for direct screen access or
 *  frame buffer drawing
 *  (assuming a fixed 160x128 pixel screen)
 *
 */
void        lcdFillScreen(uint8_t*, uint16_t);                      // fill screen with a solid color
void        lcdDrawPixel(uint8_t*, int, int, uint16_t);             // draw a pixel
void        lcdDrawLine(uint8_t*, int, int, int, int, uint16_t);    // draw a line
void        lcdDrawChar(uint8_t*, uint16_t, uint16_t, char, uint16_t, uint16_t, int, int); // write character to LCD or buffer

#endif  /* end __pilcd_h__ */

