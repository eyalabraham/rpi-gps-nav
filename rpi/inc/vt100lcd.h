/*
 *  vt100lcd.h
 *
 *     VT100 API header for ST7735 LCD
 *
 * source: http://www.termsys.demon.co.uk/vtansi.htm
 *
 *  ANSI escape sequence is a sequence of ASCII characters,
 *  the first two of which are the ASCII "Escape" character 27 (1Bh)
 *  and the left-bracket character " [ " (5Bh).
 *  The character or characters following the escape and left-bracket characters
 *  specify an alphanumeric code that controls a keyboard or display function.
 *
 * + Enable Line Wrap    <ESC>[7h                Text wraps to next line if longer than the length of the display area.
 * + Disable Line Wrap   <ESC>[7l                Disables line wrapping.
 * + Cursor Home         <ESC>[{ROW};{COLUMN}H   Sets the cursor position where subsequent text will begin.
 *                                               If no row/column parameters are provided (ie. <ESC>[H),
 *                                               the cursor will move to the home position, at the upper left of the screen.
 * + Cursor Up           <ESC>[{COUNT}A          Moves the cursor up by COUNT rows; the default count is 1.
 * + Cursor Down         <ESC>[{COUNT}B          Moves the cursor down by COUNT rows; the default count is 1.
 * + Cursor Forward      <ESC>[{COUNT}C          Moves the cursor forward by COUNT columns; the default count is 1.
 * + Cursor Backward     <ESC>[{COUNT}D          Moves the cursor backward by COUNT columns; the default count is 1.
 * + Set Cursor Position <ESC>[{ROW};{COL}f      Identical to Cursor Home.
 * + Save Cursor         <ESC>[s                 Save current cursor position.
 * + Unsave Cursor       <ESC>[u                 Restores cursor position after a Save Cursor.
 * + Erase End of Line   <ESC>[K                 Erases from the current cursor position to the end of the current line.
 * + Erase Start of Line <ESC>[1K                Erases from the current cursor position to the start of the current line.
 * + Erase Line          <ESC>[2K                Erases the entire current line.
 *   Erase Down          <ESC>[J                 Erases the screen from the current line down to the bottom of the screen.
 *   Erase Up            <ESC>[1J                Erases the screen from the current line up to the top of the screen.
 * + Erase Screen        <ESC>[2J                Erases the screen with the background color and moves the cursor to home.
 * + Set Attribute Mode  <ESC>[{Fg};{Bg}m        Sets multiple foreground and background color attribute (reduced functionality)
 *
 *          Foreground Colors
 *      30  Black
 *      31  Red
 *      32  Green
 *      33  Yellow
 *      34  Blue
 *      35  Magenta
 *      36  Cyan
 *      37  White
 *
 *          Background Colors
 *      40  Black
 *      41  Red
 *      42  Green
 *      43  Yellow
 *      44  Blue
 *      45  Magenta
 *      46  Cyan
 *      47  White
 */

#ifndef     __vt100lcd_h__
#define     __vt100lcd_h__

#include    <stdint.h>

/* -----------------------------------------
   API definitions
----------------------------------------- */
#define     VT100_PORTRAIT      0
#define     VT100_LANDSCAPE     1

#define     VT100_ESC           27

/* -----------------------------------------
   function prototypes
----------------------------------------- */
void    vt100_lcd_init(int, int, uint16_t, uint16_t);       // initialize the module with display orientation and background/foreground colors
int     vt100_lcd_columns(void);                            // get LCD text columns
int     vt100_lcd_rows(void);                               // get LCD text rows
void    vt100_lcd_putc(uint8_t*, int, char);                // output a character through VT100 driver
int     vt100_lcd_printf(uint8_t*, int, const char*, ...);  // printf style command for text output to LCD or frame buffer,
                                                            // with VT100 support

#endif  /* __vt100lcd_h__ */
