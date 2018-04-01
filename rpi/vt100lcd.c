/*
 *  vt100lcd.c
 *
 *      VT100 API implementation for ST7735 LCD
 *
 */

#define     VT100_API_DBG       0

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <assert.h>
#include    <stdarg.h>

#include    "pilcd.h"                   // ST7735 1.8" LCD driver
#include    "vt100lcd.h"                // VT100 API

/* -----------------------------------------
   module macros and types
----------------------------------------- */
#define     ASCII_ESC       27
#define     ASCII_CR        13
#define     ASCII_LF        10
#define     ASCII_BS        8
#define     ASCII_SPC       32

#define     ESC_NONE        0           // used in vt100_lcd_putc()
#define     ESC_FOUND       1
#define     ESC_BRACKET     2

#define     CODE_BUFF       32          // used in parseEscapeSeq()
#define     STATE_NONE      ESC_NONE
#define     STATE_DIGIT     ESC_BRACKET

#define     VT100_LINE_LEN  80

/* -----------------------------------------
   module globals
----------------------------------------- */
static uint8_t          vt100cursCol;
static uint8_t          vt100cursRow;
static uint8_t          vt100saveCursCol;
static uint8_t          vt100saveCursRow;
static uint16_t         vt100backgroundColor;
static uint16_t         vt100foregroundColor;
static uint8_t          vt100maxRows;
static uint8_t          vt100maxCols;
static uint8_t          vt100lineWrap;
static int              vt100fontScale;

/* -----------------------------------------
   static functions
----------------------------------------- */
static uint8_t  parseEscapeSeq(uint8_t*, int, char);
static void     getEscapeParam(char*, int*, int*);
static uint16_t converToColor(int);
static void     clearScreen(uint8_t*);

/* -----------------------------------------
   API functions
----------------------------------------- */

/*------------------------------------------------
 * vt100_lcd_init()
 *
 *  initialize the module with display orientation and background/foreground colors
 *
 * param:  display rotation for landscape or portrait, background and foreground colors
 * return: none
 */
void vt100_lcd_init(int rotation, int scale, uint16_t bg, uint16_t fg)
{
    // initialize display rotation
    lcdSetRotation(rotation);
    
    // initialize internal module variable
    vt100cursCol = 0;
    vt100cursRow = 0;
    vt100saveCursCol = 0;
    vt100saveCursRow = 0;
    vt100backgroundColor = bg;
    vt100foregroundColor = fg;
    vt100lineWrap = 0;
    vt100maxRows = lcdHeight() / FONT_PIX_HIGH / scale;
    vt100maxCols = lcdWidth() / FONT_PIX_WIDE / scale;
    vt100fontScale = scale;
}

/*------------------------------------------------
 * vt100_lcd_columns()
 *
 *  return count of LCD text columns
 *
 * param:  none
 * return: number of text columns
 */
int vt100_lcd_columns(void)
{
    return (int) vt100maxCols;
}

/*------------------------------------------------
 * vt100_lcd_rows()
 *
 *  return count of LCD text rows
 *
 * param:  none
 * return: number of text columns
 */
int vt100_lcd_rows(void)
{
    return (int) vt100maxRows;
}

/*------------------------------------------------
 * vt100_lcd_putc()
 *
 *  output a character through VT100 driver
 *
 * param:  frameBuff   pointer to allocated frame buffer, if NULL the function writes direct to screen
 *         transparent '0' paint background color, '1' transparent background only when writing to buffer
 *         character code to output, and can include VT100 escape codes to change color and move cursor
 * return: none
 */
void vt100_lcd_putc(uint8_t* frameBuff, int transparent, char c)
{
    uint16_t        x, y;
    static uint8_t  vt100escape = ESC_NONE;

    /* parse input character for VT100 escape code and process
     * code actions. if no code actions print a plain character to LCD
     */
    if ( vt100escape == ESC_BRACKET )
    {
        vt100escape = parseEscapeSeq(frameBuff, transparent, c);
    }

    /* is an escape character was found, check if this next character is a '['
     * if it is, then we have a valid Escape sequence so next characters
     * will be processed as escape code.
     * Otherwise, reset the escape condition.
     */
    else if ( vt100escape == ESC_FOUND )
    {
        if ( c == '[' )
        {
            vt100escape = ESC_BRACKET;
        }
        else
        {
            vt100escape = ESC_NONE;
        }
    }

    /* if found escape character then flag for processing
     * and get next characters in order to parse the VT100
     * command
     */
    else if ( c == ASCII_ESC )
    {
        vt100escape = ESC_FOUND;
    }

    /* calculate LCD screen coordinate from VT100 row and column
     * then write character to screen
     */
    else
    {
        switch ( c )
        {
            case ASCII_CR:
                vt100cursCol = 0;
                break;

            case ASCII_LF:
                vt100cursRow++;
                if ( vt100cursRow == vt100maxRows )
                    vt100cursRow--;
                break;

            case ASCII_BS:
                if ( vt100cursCol > 0 )
                    vt100cursCol--;
                break;

            default:
                x = vt100cursCol * FONT_PIX_WIDE;
                y = vt100cursRow * FONT_PIX_HIGH;

                lcdDrawChar(frameBuff, x, y, c, vt100foregroundColor, vt100backgroundColor, vt100fontScale, transparent);
                vt100cursCol++;

                /* handle behavior at end of line. either wrap text to next row
                 * or peg cursor to end of current row
                 */
                if ( vt100cursCol == vt100maxCols )
                {
                    if ( vt100lineWrap )
                    {
                        vt100cursCol = 0;
                        vt100cursRow++;
                        if ( vt100cursRow == vt100maxRows )
                            vt100cursRow--;
                    }
                    else
                    {
                        vt100cursCol--;
                    }
                }
        }
    }
}

/*------------------------------------------------
 * vt100_lcd_printf()
 *
 * A printf() style command for text output to LCD or frame buffer,
 * with VT100 support.
 *
 * param:  frameBuff   pointer to allocated frame buffer, if NULL the function writes direct to screen
 *         transparent '0' paint background color, '1' transparent background only when writing to buffer
 *         format      A printf() style format string including VT100 escape codes
 *         ...         Zero or more variables to format and print
 * return: Number of characters actually printed, '-1' on failure
 *
 */
int vt100_lcd_printf(uint8_t* frameBuff, int transparent, const char* format, ...)
{
    va_list arg;
    int     i = 0;
    int     char_printer;
    char    str[VT100_LINE_LEN] = {0};

    va_start(arg, format);

    char_printer = vsnprintf(str, VT100_LINE_LEN, format, arg);
    while ( str[i] )
    {
        vt100_lcd_putc(frameBuff, transparent, str[i]);
        i++;
    }

    va_end(arg);

    return char_printer;
}

/*------------------------------------------------
 * parseEscapeSeq()
 *
 *  this function will accumulate escape code characters,
 *  parse the string and take action on the VT100 codes.
 *  The function returns the state of escape processing as
 *  0 = done, 2 = need more characters
 *  The function receives the characters after the <ESC>[ sequence has
 *  been validated
 *
 *  The function changes the following globals:
 *  - vt100cursCol
 *  - vt100cursRow
 *  - vt100saveCursCol
 *  - vt100saveCursRow
 *  - vt100foregroundColor
 *  - vt100backgroundColor
 *  - vt100lineWrap
 *
 * param:  frameBuff   pointer to allocated frame buffer, if NULL the function writes direct to screen
 *         transparent '0' paint background color, '1' transparent background only when writing to buffer
 *         character code
 * return: ESC_NONE = done, ESC_BRACKET = need more characters
 */
static uint8_t parseEscapeSeq(uint8_t* frameBuff, int transparent, char c)
{
    static uint8_t  parseState = STATE_NONE;
    static char     codeString[CODE_BUFF];
    static uint8_t  i;

    uint8_t         j, st, en;
    uint16_t        x, y;
    int             n1, n2;

    /* initialize the VT100 code buffer before using it
     * to accumulate a new code
     */
    if ( parseState == STATE_NONE)
    {
        i = 0;
        memset(codeString, 0, CODE_BUFF);
    }

    /* first accumulate any digits if they appear right after
     * '[' bracket character
     */
    if ( (c >= '0' && c <= '9') || (c == ';') )
    {
        parseState = STATE_DIGIT;
        if ( i < CODE_BUFF )
        {
            codeString[i] = c;
            i++;
        }
    }

    /* if a letter is encountered we're either done with digits
     * or had no digits at all. either way, the letter is the
     * last character of a sequence so now is the time to execute
     * the VT100 command.
     */
    else
    {
        parseState = STATE_NONE;
        getEscapeParam(codeString, &n1, &n2);

        switch ( c )
        {
            /*  Enable Line Wrap    <ESC>[7h                Text wraps to next line if longer than the length of the display area.
             */
            case 'h':
                if ( n1 == 7 )
                {
                    vt100lineWrap = 1;
                }
                break;

            /*  Disable Line Wrap   <ESC>[7l                Disables line wrapping.
             */
            case 'l':
                if ( n1 == 7 )
                {
                    vt100lineWrap = 0;
                }
                break;

            /*  Cursor Home         <ESC>[{ROW};{COLUMN}H   Sets the cursor position where subsequent text will begin.
             *                                              If no row/column parameters are provided (ie. <ESC>[H),
             *                                              the cursor will move to the home position, at the upper left of the screen.
             *  Set Cursor Position <ESC>[{ROW};{COL}f      Identical to Cursor Home.
             */
            case 'f':
            case 'H':
                if ( n1 >= 0 && n1 < vt100maxRows )
                    vt100cursRow = (uint8_t)n1;

                if ( n2 >= 0 && n2 < vt100maxCols )
                    vt100cursCol = (uint8_t)n2;

                if ( n1 == -1 && n2 == -1 )
                {
                    vt100cursRow = 0;
                    vt100cursCol = 0;
                }
                break;

            /*  Cursor Up           <ESC>[{COUNT}A          Moves the cursor up by COUNT rows; the default count is 1.
             */
            case 'A':
                if ( n1 == -1 )
                    n1 = 1;

                if ( n1 > vt100cursRow )
                    vt100cursRow = 0;
                else
                    vt100cursRow -= n1;
                break;

            /*  Cursor Down         <ESC>[{COUNT}B          Moves the cursor down by COUNT rows; the default count is 1.
             */
            case 'B':
                if ( n1 == -1 )
                    n1 = 1;

                if ( (n1 + vt100cursRow) >= vt100maxRows )
                    vt100cursRow = vt100maxRows;
                else
                    vt100cursRow += n1;
                break;

            /*  Cursor Forward      <ESC>[{COUNT}C          Moves the cursor forward by COUNT columns; the default count is 1.
             */
            case 'C':
                if ( n1 == -1 )
                    n1 = 1;

                if ( (n1 + vt100cursCol) >= vt100maxCols )
                    vt100cursCol = vt100maxCols;
                else
                    vt100cursCol += n1;
                break;

            /*  Cursor Backward     <ESC>[{COUNT}D              Moves the cursor backward by COUNT columns; the default count is 1.
             */
            case 'D':
                if ( n1 == -1 )
                    n1 = 1;

                if ( n1 > vt100cursCol )
                    vt100cursCol = 0;
                else
                    vt100cursCol -= n1;
                break;

            /*  Save Cursor         <ESC>[s                 Save current cursor position.
             */
            case 's':
                vt100saveCursCol = vt100cursCol;
                vt100saveCursRow = vt100cursRow;
                break;
            /*  Unsave Cursor       <ESC>[u                 Restores cursor position after a Save Cursor.
             */
            case 'u':
                vt100cursCol = vt100saveCursCol;
                vt100cursRow = vt100saveCursRow;
                break;

            /*  Erase End of Line   <ESC>[K                 Erases from the current cursor position to the end of the current line.
             *  Erase Start of Line <ESC>[1K                Erases from the current cursor position to the start of the current line.
             *  Erase Line          <ESC>[2K                Erases the entire current line.
             */
            case 'K':
                if ( n1 == 1 )
                {
                    st = 0;
                    en = vt100cursCol;
                }
                else if ( n1 == 2 )
                {
                    st = 0;
                    en = vt100maxCols;
                }
                else
                {
                    st = vt100cursCol;
                    en = vt100maxCols;
                }

                for ( j = st; j < en; j++ )
                {
                    x = j * FONT_PIX_WIDE;
                    y = vt100cursRow * FONT_PIX_HIGH;
                    lcdDrawChar(frameBuff, x, y, ASCII_SPC, vt100foregroundColor, vt100backgroundColor, vt100fontScale, transparent);
                }
                break;

            /*  Erase Down          <ESC>[J                 Erases the screen from the current line down to the bottom of the screen.
             *  Erase Up            <ESC>[1J                Erases the screen from the current line up to the top of the screen.
             *  Erase Screen        <ESC>[2J                Erases the screen with the background color and moves the cursor to home.
             */
            case 'J':
                if ( n1 == 2 )
                {
                    clearScreen(frameBuff);
                    vt100cursCol = 0;
                    vt100cursRow = 0;
                }
                break;

            /*  Set Attribute Mode  <ESC>[{Fg};{Bg}m        Sets multiple foreground and background color attribute (reduced functionality)
             */
            case 'm':
                /* convert n1 and/or n2 integer parameters into
                 * the color codes for background and foreground
                 */
                if ( n1 < 30 || n1 > 47 )
                    break;
                else if ( n1 <= 37 )
                    vt100foregroundColor = converToColor(n1-30);
                else if ( n1 >= 40 )
                    vt100backgroundColor = converToColor(n1-40);

                if ( n2 < 30 || n2 > 47 )
                    break;
                else if ( n2 <= 37 )
                    vt100foregroundColor = converToColor(n2-30);
                else if ( n2 >= 40 )
                    vt100backgroundColor = converToColor(n2-40);
                break;

            default:
#if VT100_API_DBG
                printf("parseEscapeSeq(<esc>[%s%c) bad ESC sequence\n", codeString, c);
#endif
                ;;
        }
    }

    return parseState;
}

/*------------------------------------------------
 * getEscapeParam()
 *
 *  The function get a pointer to a zero terminated
 *  string that may contain one, two or no decimal integers.
 *  The integers are separated by a semicolon.
 *
 * param:  pointer to zero terminated string, pointer to two integers
 * return: none
 */
static void getEscapeParam(char* s, int* n1, int* n2)
{
    int     temp, p, i;

    *n1 = -1;
    *n2 = -1;

    if ( strlen(s) == 0 )
        return;

    temp = -1;
    p = 1;

    for ( i = (strlen(s) - 1); i >= 0; i-- )
    {
        if ( s[i] == ';' )
        {
            *n2 = temp;
            temp = -1;
            p = 1;
            continue;
        }

        if ( temp == -1 )
            temp = 0;
        temp += (s[i] - '0') * p;
        p *= 10;
    }

    *n1 = temp;

#if VT100_API_DBG
    printf("getEscapeParam(%d, %d)\n", *n1, *n2);
#endif
}

/*------------------------------------------------
 * converToColor()
 *
 *  convert the integer color code into the LCD
 *  color value
 *
 * param:  VT100 color code with '30' or '40' already subtracted
 * return: LCD color value
 *
 */
static uint16_t converToColor(int vt100colorCode)
{
    uint16_t    color;

    switch ( vt100colorCode )
    {
        case 0:
            color = ST7735_BLACK;
            break;
        case 1:
            color = ST7735_RED;
            break;
        case 2:
            color = ST7735_GREEN;
            break;
        case 3:
            color = ST7735_YELLOW;
            break;
        case 4:
            color = ST7735_BLUE;
            break;
        case 5:
            color = ST7735_MAGENTA;
            break;
        case 6:
            color = ST7735_CYAN;
            break;
        case 7:
            color = ST7735_WHITE;
            break;
        default:;
    }

    return color;
}

/*------------------------------------------------
 * clearScreen()
 *
 *  clear screen to background color
 *
 * param:  frameBuff   pointer to allocated frame buffer, if NULL the function writes direct to screen
 * return: none
 */
static void clearScreen(uint8_t* frameBuff)
{
    uint8_t*    screen;

    if ( frameBuff )
    {
        lcdFrameBufferColor(frameBuff, vt100backgroundColor);
        lcdFrameBufferPush(frameBuff);
    }
    else
    {
        screen = lcdFrameBufferInit(vt100backgroundColor);
        assert(screen);
        lcdFrameBufferPush(screen);
        lcdFrameBufferFree(screen);
    }
}
