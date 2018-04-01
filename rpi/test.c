/********************************************************************
 * test.c
 *
 *  Module test.c containing test routines called from the main module
 *  with -t <test_num> command line argument.
 *  Test are written as a single function per test with a return value
 *  of '0' for pass or '1' for failure. The functions are executed as
 *  a single unit assuming no other code will run after the test exits,
 *  and that the main module will exit after test execution completes.
 *
 *  March 15, 2018
 *
 *******************************************************************/

#include    <ctype.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <fcntl.h>
#include    <errno.h>
#include    <termios.h>
#include    <string.h>

#include    "test.h"
#include    "pilcd.h"
#include    "vt100lcd.h"
#include    "util.h"
#include    "config.h"

/********************************************************************
 * Definitions and globals
 *
 */
#define     FRAME_BUFF_SIZE (ST7735_TFTWIDTH*ST7735_TFTHEIGHT)
#define     PATTERN1_FILE   "res/pattern1.raw"
#define     PATTERN2_FILE   "res/pattern2.raw"

static union frame_buffer_t
{
    uint16_t pixel_words[FRAME_BUFF_SIZE];
    uint8_t  pixel_bytes[2*FRAME_BUFF_SIZE];
} frame_buffer;

/********************************************************************
 * test_t0_lcd()
 *
 *  Initialize LCD and display a series of images before exiting.
 *
 *  param:  none
 *  return: 0 if no error, 
 *         -1 if error initializing any subcomponent or library
 *
 */
int test_t0_lcd(void)
{
    int fd;
    
    printf("Test t0\n");
    
    // try to initialize GPIO subsystem
    printf("  Initializing GPIO\n");
    if ( !bcm2835_init() )
    {
        printf("  bcm2835_init failed. Are you running as root?\n");
        return -1;
    }
    
    // Initialize RST GPIO pin
    bcm2835_gpio_fsel(LCD_RST, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(LCD_RST, HIGH);
 
    // Initialize SPI
    printf("  Initializing SPI\n");
    if (!bcm2835_spi_begin())
    {
        printf("  bcm2835_spi_begin failed. Are you running as root??\n");
        return -1;
    }
    
    // Initialize SPI for the LCD according to wiring
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    // Reset the devices on the SPI bus
    bcm2835_gpio_write(LCD_RST, LOW);
    bcm2835_delay(250);
    bcm2835_gpio_write(LCD_RST, HIGH);
    
    // LCD initialization and test
    printf("  Testing LCD display\n");
    lcdInit();
    lcdSetRotation(3);

    printf("  Red\n");
    lcdFrameBufferColor(frame_buffer.pixel_bytes, ST7735_RED);
    lcdFrameBufferPush(frame_buffer.pixel_bytes);
    bcm2835_delay(2000);
    
    printf("  Green\n");
    lcdFrameBufferColor(frame_buffer.pixel_bytes, ST7735_GREEN);
    lcdFrameBufferPush(frame_buffer.pixel_bytes);
    bcm2835_delay(2000);

    printf("  Blue\n");
    lcdFrameBufferColor(frame_buffer.pixel_bytes, ST7735_BLUE);
    lcdFrameBufferPush(frame_buffer.pixel_bytes);
    bcm2835_delay(2000);

    fd = open(PATTERN1_FILE, O_RDONLY);
    if ( fd == -1 )
    {
        printf("  Error %d opening image file %s\n", errno, PATTERN1_FILE);
        return -1;
    }
    else
    {
        printf("  %s\n", PATTERN1_FILE);
        read(fd, (void*) frame_buffer.pixel_bytes, (2*FRAME_BUFF_SIZE));
        close(fd);
        lcdFrameBufferPush(frame_buffer.pixel_bytes);
        bcm2835_delay(2000);
    }
 
    fd = open(PATTERN2_FILE, O_RDONLY);
    if ( fd == -1 )
    {
        printf("  Error %d opening image file %s\n", errno, PATTERN1_FILE);
        return -1;
    }
    else
    {
        printf("  %s\n", PATTERN2_FILE);
        read(fd, (void*) frame_buffer.pixel_bytes, (2*FRAME_BUFF_SIZE));
        close(fd);
        lcdFrameBufferPush(frame_buffer.pixel_bytes);
        bcm2835_delay(2000);
    }
    
    lcdFrameBufferColor(frame_buffer.pixel_bytes, ST7735_BLACK);
    lcdFrameBufferPush(frame_buffer.pixel_bytes);

    //lcdOff();
    
    printf("Done\n");
    
    // Close SPI
    bcm2835_spi_end();

    // Close GPIO
    bcm2835_close();

    return 0;
}

/********************************************************************
 * test_t1_pbuttons()
 *
 *  Initialized pushbutton input GPIO lines
 *
 *  param:  none
 *  return: 0 if no error, 
 *         -1 if error initializing any subcomponent or library
 *
 */
int test_t1_pbuttons(void)
{
    int     i = 0;

    printf("Test t1\n");
    
    // try to initialize GPIO subsystem
    printf("  Initializing GPIO\n");
    if ( !bcm2835_init() )
    {
        printf("  bcm2835_init failed. Are you running as root?\n");
        return -1;
    }

    // Initialize GPIO pins for input with pull-up enabled
    bcm2835_gpio_fsel(PBUTTON_UP, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PBUTTON_UP, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(PBUTTON_DOWN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PBUTTON_DOWN, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(PBUTTON_LEFT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PBUTTON_LEFT, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(PBUTTON_RIGHT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PBUTTON_RIGHT, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(PBUTTON_SELECT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PBUTTON_SELECT, BCM2835_GPIO_PUD_UP);


    // Loop here and report pushbuttons that are pressed
    // exit after 10 readouts
    printf("  Will quit after 20 presses\n");
    while ( i < 20 )
    {
        if ( bcm2835_gpio_lev(PBUTTON_UP) == LOW )
        {
            printf("  %2d UP\n", i);
            i++;
            bcm2835_delay(500);
        }
        else if ( bcm2835_gpio_lev(PBUTTON_DOWN) == LOW )
        {
            printf("  %2d DOWN\n", i);
            i++;
            bcm2835_delay(500);
        }
        else if ( bcm2835_gpio_lev(PBUTTON_LEFT) == LOW )
        {
            printf("  %2d LEFT\n", i);
            i++;
            bcm2835_delay(500);
        }
        else if ( bcm2835_gpio_lev(PBUTTON_RIGHT) == LOW )
        {
            printf("  %2d RIGHT\n", i);
            i++;
            bcm2835_delay(500);
        }
        else if ( bcm2835_gpio_lev(PBUTTON_SELECT) == LOW )
        {
            printf("  %2d SELECT\n", i);
            i++;
            bcm2835_delay(500);
        }
    }

    // Close GPIO
    bcm2835_close();

    return 0;
}

/********************************************************************
 * test_t2_gps()
 *
 *  Connect to GPS module through UART and report 10 NMEA sentences.
 *
 *  param:  none
 *  return: 0 if no error, 
 *         -1 if error initializing any subcomponent or library
 *
 */
int test_t2_gps(void)
{
    int     newline_count = 0;
    int     uart_fd;
    int     read_result;
    char    nmea_text[512] = {0};
    struct  position_t  pos;
    int     valid_fix;
    
    printf("Test t2\n");
    
    // Open UART0 port
    uart_fd = open(UART0, O_RDWR | O_NOCTTY | O_NDELAY);
    if ( uart_fd == -1 )
    {
        printf("  Error %d opening %s\n", errno, UART0);
        return -1;
    }
    else
    {
        printf("  Initializing UART0\n");
        
        // Setup UART options
        uart_set_interface_attr(uart_fd, B9600, 0);
        uart_set_blocking(uart_fd, 0);
        
        // Read some data and print to stdout
        fcntl(uart_fd, F_SETFL, FNDELAY);
        
        memset(&pos, 0, sizeof(struct  position_t));

        while ( newline_count < 60 )
        {
            // try to read a text line from the UART
            read_result = uart_read_line(uart_fd, nmea_text, 512);

            // if an error occurred, then abort
            if ( read_result < 0 )
            {
                printf("  Error %d reading UART %s\n", errno, UART0);
                break;
            }

            // nothing read, keep waiting
            else if ( read_result == 0 )
            {
                continue;
            }

            // only a valid text line can be present at this point
            else
            {
                valid_fix = nmea_update_pos(nmea_text, &pos);
                printf("%s |%s|\n", valid_fix ? "[ok ]" : "[err]", nmea_text);
                if ( valid_fix )
                {
                    // print position information
                    printf("      UTC Time %02d:%02d:%#-6.3g\n", pos.hour, pos.min, pos.sec);
                    printf("      Latitude %#-9.6g\n", pos.latitude);
                    printf("      Longitude %#-9.6g\n", pos.longitude);
                    printf("      Satellites %d\n", pos.sat_count);
                    printf("      Ground speed %g [mph]\n", pos.ground_spd);
                    printf("      Heading %g [deg]\n", pos.heading);
                }
                newline_count++;
            }
        }

        // Close the port and exit
        close(uart_fd);
    }
    
    printf("Done\n");
    
    return 0;
}

