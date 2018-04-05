/********************************************************************
 * nav.c
 *
 *  Module nav.c is the main application module containing the code
 *  for map-based GPS navigation.
 *
 *  March 21, 2018
 *
 *******************************************************************/

#include    <ctype.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <string.h>
#include    <fcntl.h>
#include    <errno.h>
#include    <termios.h>
#include    <math.h>

#include    "nav.h"
#include    "pilcd.h"
#include    "vt100lcd.h"
#include    "util.h"
#include    "config.h"

/********************************************************************
 * Definitions
 *
 */

#define     __FAKE_VALID_FIX__  0               // Use for map position debug

// System font color definition
#define     SYS_FONT_INV        "\e[30;47m"     // black on white
#define     SYS_FONT_NORM       "\e[37;40m"     // white on black
#define     SYS_FG_COLOR        ST7735_WHITE
#define     SYS_BG_COLOR        ST7735_BLACK
#define     SYS_FONT_SCALE      1

// Operation status messages
#define     STATUS_OK           "[ OK ]"
#define     STATUS_FAIL         "[\e[1;31mFAIL\e[0m]"
#define     GREETING            "\e[HRaspberry Pi GPS Nav.\r\n"     \
                                "Revision 1.0, Mar. 24 2018\r\n"    \
                                "Eyal Abraham (c)"
#define     NOT_IMPLEMENTED     "\e[8;2f\e[31;40mNOT IMPLEMENTED"
#define     FRAME_BUFF_SIZE     (ST7735_TFTWIDTH*ST7735_TFTHEIGHT)

// Navigator state
#define     STATE_INIT          0
#define     STATE_MAIN_MENU     1
#define     STATE_GPS_DATA      2
#define     STATE_MAP_NAV       3
#define     STATE_LOGGER        4
#define     STATE_EXIT          5

// Main menu
#define     MAIN_MENU_TOP       0
#define     MAIN_MENU_MAP       MAIN_MENU_TOP
#define     MAIN_MENU_GPS_DAT   1
#define     MAIN_MENU_GPR_TRK   2
#define     MAIN_MENU_SHUTDW    MAIN_MENU_BOTTOM
#define     MAIN_MENU_BOTTOM    3

// GO and Logger
#define     USB_DIR             "/home/pi/usb"
#define     GO_FILE             "/home/pi/usb/go"
#define     LOGGER_FILE         "/home/pi/usb/logger.csv"
#define     MAP_XML_FILE        "/home/pi/usb/maps.xml"
//#define     MAP_XML_FILE        "/home/pi/usb/sample.xml"

/********************************************************************
 * Static function prototypes
 *
 */
static int  gpio_init(void);
static void gpio_shutdown(void);
static void menu_print(int);
static void msg_not_implemented(void);
static void gps_data(int);
static void gps_map_nav(void);
static uint16_t *load_map_image(struct map_t *);
static void get_map_patch(struct position_t *, struct map_t *, uint16_t *);

/********************************************************************
 * Static SIN() and COS() tables for integer angles in *degrees*
 *
 */
static double SIN[] = {
        0.0,
        0.0174524064373,
        0.0348994967025,
        0.0523359562429,
        0.0697564737441,
        0.0871557427477,
        0.104528463268,
        0.121869343405,
        0.13917310096,
        0.15643446504,
        0.173648177667,
        0.190808995377,
        0.207911690818,
        0.224951054344,
        0.2419218956,
        0.258819045103,
        0.275637355817,
        0.292371704723,
        0.309016994375,
        0.325568154457,
        0.342020143326,
        0.358367949545,
        0.374606593416,
        0.390731128489,
        0.406736643076,
        0.422618261741,
        0.438371146789,
        0.45399049974,
        0.469471562786,
        0.484809620246,
        0.5,
        0.51503807491,
        0.529919264233,
        0.544639035015,
        0.559192903471,
        0.573576436351,
        0.587785252292,
        0.601815023152,
        0.615661475326,
        0.62932039105,
        0.642787609687,
        0.656059028991,
        0.669130606359,
        0.681998360062,
        0.694658370459,
        0.707106781187,
        0.719339800339,
        0.731353701619,
        0.743144825477,
        0.754709580223,
        0.766044443119,
        0.777145961457,
        0.788010753607,
        0.798635510047,
        0.809016994375,
        0.819152044289,
        0.829037572555,
        0.838670567945,
        0.848048096156,
        0.857167300702,
        0.866025403784,
        0.874619707139,
        0.882947592859,
        0.891006524188,
        0.898794046299,
        0.906307787037,
        0.913545457643,
        0.920504853452,
        0.927183854567,
        0.933580426497,
        0.939692620786,
        0.945518575599,
        0.951056516295,
        0.956304755963,
        0.961261695938,
        0.965925826289,
        0.970295726276,
        0.974370064785,
        0.978147600734,
        0.981627183448,
        0.984807753012,
        0.987688340595,
        0.990268068742,
        0.992546151641,
        0.994521895368,
        0.996194698092,
        0.99756405026,
        0.998629534755,
        0.999390827019,
        0.999847695156,
        1.0,
        0.999847695156,
        0.999390827019,
        0.998629534755,
        0.99756405026,
        0.996194698092,
        0.994521895368,
        0.992546151641,
        0.990268068742,
        0.987688340595,
        0.984807753012,
        0.981627183448,
        0.978147600734,
        0.974370064785,
        0.970295726276,
        0.965925826289,
        0.961261695938,
        0.956304755963,
        0.951056516295,
        0.945518575599,
        0.939692620786,
        0.933580426497,
        0.927183854567,
        0.920504853452,
        0.913545457643,
        0.906307787037,
        0.898794046299,
        0.891006524188,
        0.882947592859,
        0.874619707139,
        0.866025403784,
        0.857167300702,
        0.848048096156,
        0.838670567945,
        0.829037572555,
        0.819152044289,
        0.809016994375,
        0.798635510047,
        0.788010753607,
        0.777145961457,
        0.766044443119,
        0.754709580223,
        0.743144825477,
        0.731353701619,
        0.719339800339,
        0.707106781187,
        0.694658370459,
        0.681998360062,
        0.669130606359,
        0.656059028991,
        0.642787609687,
        0.62932039105,
        0.615661475326,
        0.601815023152,
        0.587785252292,
        0.573576436351,
        0.559192903471,
        0.544639035015,
        0.529919264233,
        0.51503807491,
        0.5,
        0.484809620246,
        0.469471562786,
        0.45399049974,
        0.438371146789,
        0.422618261741,
        0.406736643076,
        0.390731128489,
        0.374606593416,
        0.358367949545,
        0.342020143326,
        0.325568154457,
        0.309016994375,
        0.292371704723,
        0.275637355817,
        0.258819045103,
        0.2419218956,
        0.224951054344,
        0.207911690818,
        0.190808995377,
        0.173648177667,
        0.15643446504,
        0.13917310096,
        0.121869343405,
        0.104528463268,
        0.0871557427477,
        0.0697564737441,
        0.0523359562429,
        0.0348994967025,
        0.0174524064373,
        1.22464679915e-16,
        -0.0174524064373,
        -0.0348994967025,
        -0.0523359562429,
        -0.0697564737441,
        -0.0871557427477,
        -0.104528463268,
        -0.121869343405,
        -0.13917310096,
        -0.15643446504,
        -0.173648177667,
        -0.190808995377,
        -0.207911690818,
        -0.224951054344,
        -0.2419218956,
        -0.258819045103,
        -0.275637355817,
        -0.292371704723,
        -0.309016994375,
        -0.325568154457,
        -0.342020143326,
        -0.358367949545,
        -0.374606593416,
        -0.390731128489,
        -0.406736643076,
        -0.422618261741,
        -0.438371146789,
        -0.45399049974,
        -0.469471562786,
        -0.484809620246,
        -0.5,
        -0.51503807491,
        -0.529919264233,
        -0.544639035015,
        -0.559192903471,
        -0.573576436351,
        -0.587785252292,
        -0.601815023152,
        -0.615661475326,
        -0.62932039105,
        -0.642787609687,
        -0.656059028991,
        -0.669130606359,
        -0.681998360062,
        -0.694658370459,
        -0.707106781187,
        -0.719339800339,
        -0.731353701619,
        -0.743144825477,
        -0.754709580223,
        -0.766044443119,
        -0.777145961457,
        -0.788010753607,
        -0.798635510047,
        -0.809016994375,
        -0.819152044289,
        -0.829037572555,
        -0.838670567945,
        -0.848048096156,
        -0.857167300702,
        -0.866025403784,
        -0.874619707139,
        -0.882947592859,
        -0.891006524188,
        -0.898794046299,
        -0.906307787037,
        -0.913545457643,
        -0.920504853452,
        -0.927183854567,
        -0.933580426497,
        -0.939692620786,
        -0.945518575599,
        -0.951056516295,
        -0.956304755963,
        -0.961261695938,
        -0.965925826289,
        -0.970295726276,
        -0.974370064785,
        -0.978147600734,
        -0.981627183448,
        -0.984807753012,
        -0.987688340595,
        -0.990268068742,
        -0.992546151641,
        -0.994521895368,
        -0.996194698092,
        -0.99756405026,
        -0.998629534755,
        -0.999390827019,
        -0.999847695156,
        -1.0,
        -0.999847695156,
        -0.999390827019,
        -0.998629534755,
        -0.99756405026,
        -0.996194698092,
        -0.994521895368,
        -0.992546151641,
        -0.990268068742,
        -0.987688340595,
        -0.984807753012,
        -0.981627183448,
        -0.978147600734,
        -0.974370064785,
        -0.970295726276,
        -0.965925826289,
        -0.961261695938,
        -0.956304755963,
        -0.951056516295,
        -0.945518575599,
        -0.939692620786,
        -0.933580426497,
        -0.927183854567,
        -0.920504853452,
        -0.913545457643,
        -0.906307787037,
        -0.898794046299,
        -0.891006524188,
        -0.882947592859,
        -0.874619707139,
        -0.866025403784,
        -0.857167300702,
        -0.848048096156,
        -0.838670567945,
        -0.829037572555,
        -0.819152044289,
        -0.809016994375,
        -0.798635510047,
        -0.788010753607,
        -0.777145961457,
        -0.766044443119,
        -0.754709580223,
        -0.743144825477,
        -0.731353701619,
        -0.719339800339,
        -0.707106781187,
        -0.694658370459,
        -0.681998360062,
        -0.669130606359,
        -0.656059028991,
        -0.642787609687,
        -0.62932039105,
        -0.615661475326,
        -0.601815023152,
        -0.587785252292,
        -0.573576436351,
        -0.559192903471,
        -0.544639035015,
        -0.529919264233,
        -0.51503807491,
        -0.5,
        -0.484809620246,
        -0.469471562786,
        -0.45399049974,
        -0.438371146789,
        -0.422618261741,
        -0.406736643076,
        -0.390731128489,
        -0.374606593416,
        -0.358367949545,
        -0.342020143326,
        -0.325568154457,
        -0.309016994375,
        -0.292371704723,
        -0.275637355817,
        -0.258819045103,
        -0.2419218956,
        -0.224951054344,
        -0.207911690818,
        -0.190808995377,
        -0.173648177667,
        -0.15643446504,
        -0.13917310096,
        -0.121869343405,
        -0.104528463268,
        -0.0871557427477,
        -0.0697564737441,
        -0.0523359562429,
        -0.0348994967025,
        -0.0174524064373
};

static double COS[] = {
        1.0,
        0.999847695156,
        0.999390827019,
        0.998629534755,
        0.99756405026,
        0.996194698092,
        0.994521895368,
        0.992546151641,
        0.990268068742,
        0.987688340595,
        0.984807753012,
        0.981627183448,
        0.978147600734,
        0.974370064785,
        0.970295726276,
        0.965925826289,
        0.961261695938,
        0.956304755963,
        0.951056516295,
        0.945518575599,
        0.939692620786,
        0.933580426497,
        0.927183854567,
        0.920504853452,
        0.913545457643,
        0.906307787037,
        0.898794046299,
        0.891006524188,
        0.882947592859,
        0.874619707139,
        0.866025403784,
        0.857167300702,
        0.848048096156,
        0.838670567945,
        0.829037572555,
        0.819152044289,
        0.809016994375,
        0.798635510047,
        0.788010753607,
        0.777145961457,
        0.766044443119,
        0.754709580223,
        0.743144825477,
        0.731353701619,
        0.719339800339,
        0.707106781187,
        0.694658370459,
        0.681998360062,
        0.669130606359,
        0.656059028991,
        0.642787609687,
        0.62932039105,
        0.615661475326,
        0.601815023152,
        0.587785252292,
        0.573576436351,
        0.559192903471,
        0.544639035015,
        0.529919264233,
        0.51503807491,
        0.5,
        0.484809620246,
        0.469471562786,
        0.45399049974,
        0.438371146789,
        0.422618261741,
        0.406736643076,
        0.390731128489,
        0.374606593416,
        0.358367949545,
        0.342020143326,
        0.325568154457,
        0.309016994375,
        0.292371704723,
        0.275637355817,
        0.258819045103,
        0.2419218956,
        0.224951054344,
        0.207911690818,
        0.190808995377,
        0.173648177667,
        0.15643446504,
        0.13917310096,
        0.121869343405,
        0.104528463268,
        0.0871557427477,
        0.0697564737441,
        0.0523359562429,
        0.0348994967025,
        0.0174524064373,
        6.12323399574e-17,
        -0.0174524064373,
        -0.0348994967025,
        -0.0523359562429,
        -0.0697564737441,
        -0.0871557427477,
        -0.104528463268,
        -0.121869343405,
        -0.13917310096,
        -0.15643446504,
        -0.173648177667,
        -0.190808995377,
        -0.207911690818,
        -0.224951054344,
        -0.2419218956,
        -0.258819045103,
        -0.275637355817,
        -0.292371704723,
        -0.309016994375,
        -0.325568154457,
        -0.342020143326,
        -0.358367949545,
        -0.374606593416,
        -0.390731128489,
        -0.406736643076,
        -0.422618261741,
        -0.438371146789,
        -0.45399049974,
        -0.469471562786,
        -0.484809620246,
        -0.5,
        -0.51503807491,
        -0.529919264233,
        -0.544639035015,
        -0.559192903471,
        -0.573576436351,
        -0.587785252292,
        -0.601815023152,
        -0.615661475326,
        -0.62932039105,
        -0.642787609687,
        -0.656059028991,
        -0.669130606359,
        -0.681998360062,
        -0.694658370459,
        -0.707106781187,
        -0.719339800339,
        -0.731353701619,
        -0.743144825477,
        -0.754709580223,
        -0.766044443119,
        -0.777145961457,
        -0.788010753607,
        -0.798635510047,
        -0.809016994375,
        -0.819152044289,
        -0.829037572555,
        -0.838670567945,
        -0.848048096156,
        -0.857167300702,
        -0.866025403784,
        -0.874619707139,
        -0.882947592859,
        -0.891006524188,
        -0.898794046299,
        -0.906307787037,
        -0.913545457643,
        -0.920504853452,
        -0.927183854567,
        -0.933580426497,
        -0.939692620786,
        -0.945518575599,
        -0.951056516295,
        -0.956304755963,
        -0.961261695938,
        -0.965925826289,
        -0.970295726276,
        -0.974370064785,
        -0.978147600734,
        -0.981627183448,
        -0.984807753012,
        -0.987688340595,
        -0.990268068742,
        -0.992546151641,
        -0.994521895368,
        -0.996194698092,
        -0.99756405026,
        -0.998629534755,
        -0.999390827019,
        -0.999847695156,
        -1.0,
        -0.999847695156,
        -0.999390827019,
        -0.998629534755,
        -0.99756405026,
        -0.996194698092,
        -0.994521895368,
        -0.992546151641,
        -0.990268068742,
        -0.987688340595,
        -0.984807753012,
        -0.981627183448,
        -0.978147600734,
        -0.974370064785,
        -0.970295726276,
        -0.965925826289,
        -0.961261695938,
        -0.956304755963,
        -0.951056516295,
        -0.945518575599,
        -0.939692620786,
        -0.933580426497,
        -0.927183854567,
        -0.920504853452,
        -0.913545457643,
        -0.906307787037,
        -0.898794046299,
        -0.891006524188,
        -0.882947592859,
        -0.874619707139,
        -0.866025403784,
        -0.857167300702,
        -0.848048096156,
        -0.838670567945,
        -0.829037572555,
        -0.819152044289,
        -0.809016994375,
        -0.798635510047,
        -0.788010753607,
        -0.777145961457,
        -0.766044443119,
        -0.754709580223,
        -0.743144825477,
        -0.731353701619,
        -0.719339800339,
        -0.707106781187,
        -0.694658370459,
        -0.681998360062,
        -0.669130606359,
        -0.656059028991,
        -0.642787609687,
        -0.62932039105,
        -0.615661475326,
        -0.601815023152,
        -0.587785252292,
        -0.573576436351,
        -0.559192903471,
        -0.544639035015,
        -0.529919264233,
        -0.51503807491,
        -0.5,
        -0.484809620246,
        -0.469471562786,
        -0.45399049974,
        -0.438371146789,
        -0.422618261741,
        -0.406736643076,
        -0.390731128489,
        -0.374606593416,
        -0.358367949545,
        -0.342020143326,
        -0.325568154457,
        -0.309016994375,
        -0.292371704723,
        -0.275637355817,
        -0.258819045103,
        -0.2419218956,
        -0.224951054344,
        -0.207911690818,
        -0.190808995377,
        -0.173648177667,
        -0.15643446504,
        -0.13917310096,
        -0.121869343405,
        -0.104528463268,
        -0.0871557427477,
        -0.0697564737441,
        -0.0523359562429,
        -0.0348994967025,
        -0.0174524064373,
        -1.83697019872e-16,
        0.0174524064373,
        0.0348994967025,
        0.0523359562429,
        0.0697564737441,
        0.0871557427477,
        0.104528463268,
        0.121869343405,
        0.13917310096,
        0.15643446504,
        0.173648177667,
        0.190808995377,
        0.207911690818,
        0.224951054344,
        0.2419218956,
        0.258819045103,
        0.275637355817,
        0.292371704723,
        0.309016994375,
        0.325568154457,
        0.342020143326,
        0.358367949545,
        0.374606593416,
        0.390731128489,
        0.406736643076,
        0.422618261741,
        0.438371146789,
        0.45399049974,
        0.469471562786,
        0.484809620246,
        0.5,
        0.51503807491,
        0.529919264233,
        0.544639035015,
        0.559192903471,
        0.573576436351,
        0.587785252292,
        0.601815023152,
        0.615661475326,
        0.62932039105,
        0.642787609687,
        0.656059028991,
        0.669130606359,
        0.681998360062,
        0.694658370459,
        0.707106781187,
        0.719339800339,
        0.731353701619,
        0.743144825477,
        0.754709580223,
        0.766044443119,
        0.777145961457,
        0.788010753607,
        0.798635510047,
        0.809016994375,
        0.819152044289,
        0.829037572555,
        0.838670567945,
        0.848048096156,
        0.857167300702,
        0.866025403784,
        0.874619707139,
        0.882947592859,
        0.891006524188,
        0.898794046299,
        0.906307787037,
        0.913545457643,
        0.920504853452,
        0.927183854567,
        0.933580426497,
        0.939692620786,
        0.945518575599,
        0.951056516295,
        0.956304755963,
        0.961261695938,
        0.965925826289,
        0.970295726276,
        0.974370064785,
        0.978147600734,
        0.981627183448,
        0.984807753012,
        0.987688340595,
        0.990268068742,
        0.992546151641,
        0.994521895368,
        0.996194698092,
        0.99756405026,
        0.998629534755,
        0.999390827019,
        0.999847695156
};

/********************************************************************
 * Module globals
 *
 */

// Main menu
static char *menu_item[] = {" Map        ", " GPS Data   ", " GPS Logger ", " Shutdown   "};

// Global variables
static int   state = STATE_INIT;
static int   usb_mounted = 0;
static int   uart_fd;
static union frame_buffer_t
{
    uint16_t pixel_words[FRAME_BUFF_SIZE];
    uint8_t  pixel_bytes[2*FRAME_BUFF_SIZE];
} frame_buffer;
static struct position_t  pos;
static struct map_t *map_list = NULL;
static uint16_t *map_image = NULL;

/********************************************************************
 * navigator()
 *
 *  Navigation function.
 *
 *  param:  none
 *  return: 0 if no error,
 *         -1 if error initializing any subcomponent or library
 *
 */
int navigator(void)
{
    int     close_navigator = 0;
    int     button_code;
    int     menu_selection;

    while ( !close_navigator )
    {
        switch ( state )
        {
            case STATE_INIT:
                printf("         %s Starting system initialization.\n", STATUS_OK);

                // Initialize IO subsystem
                if ( gpio_init() == -1 )
                {
                    printf("         %s gpio_init failed, now exiting.\n", STATUS_FAIL);
                    return -1;
                }

                // Check USB thumb drive
                if ( access(GO_FILE, F_OK) == 0 )
                {
                    usb_mounted = 1;
                }
                printf("         %s GO file checked, USB is %smounted.\n", usb_mounted ? STATUS_OK : STATUS_FAIL, usb_mounted ? "" : "not ");

                //Map database and position initialization
                if ( new_map_list(MAP_XML_FILE, &map_list) == -1 )
                    printf("         %s Map meta data parsing error.\n", STATUS_FAIL);
                else
                    printf("         %s Map meta data parsed:\n", STATUS_OK);
                dump_map_list(map_list);

                memset(&pos, 0, sizeof(struct  position_t));

                // Start navigation app state machine
                printf("         %s Starting navigation application.\n", STATUS_OK);
                state = STATE_MAIN_MENU;
                break;

            case STATE_MAIN_MENU:
                // Initialize main screen and menu
                menu_selection = MAIN_MENU_MAP;
                lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "%s", GREETING);
                menu_print(menu_selection);
                lcdFrameBufferPush(frame_buffer.pixel_bytes);

                // Loop to read push buttons and activate menu
                while ( state == STATE_MAIN_MENU )
                {
                    button_code = push_button_read();
                    switch ( button_code )
                    {
                        case PB_SELECT:
                            switch ( menu_selection )
                            {
                                case MAIN_MENU_MAP:
                                    state = STATE_MAP_NAV;
                                    break;
                                case MAIN_MENU_GPS_DAT:
                                    state = STATE_GPS_DATA;
                                    break;
                                case MAIN_MENU_GPR_TRK:
                                    state = STATE_LOGGER;
                                    break;
                                case MAIN_MENU_SHUTDW:
                                    state = STATE_EXIT;
                                    break;
                            }
                            break;

                        case PB_UP:
                            menu_selection--;
                            if ( menu_selection < MAIN_MENU_TOP )
                                menu_selection = MAIN_MENU_BOTTOM;
                            break;

                        case PB_DOWN:
                            menu_selection++;
                            if ( menu_selection > MAIN_MENU_BOTTOM )
                                menu_selection = MAIN_MENU_TOP;
                            break;

                        case PB_LEFT:
                            break;

                        case PB_RIGHT:
                            break;

                        default:;
                    }

                    // Update the menu
                    menu_print(menu_selection);
                    lcdFrameBufferPush(frame_buffer.pixel_bytes);
                }
                break;

            case STATE_GPS_DATA:
                gps_data(0);
                state = STATE_MAIN_MENU;
                break;

            case STATE_MAP_NAV:
                gps_map_nav();
                state = STATE_MAIN_MENU;
                break;

            case STATE_LOGGER:
                gps_data(1);
                state = STATE_MAIN_MENU;
                break;

            case STATE_EXIT:
                // Clear screen and exit state machine
                lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);
                lcdFrameBufferPush(frame_buffer.pixel_bytes);
                close_navigator = 1;
                break;

            default:;
        }
    }

    // Close everything and exit
    free(map_image);
    del_map_list(map_list);
    gpio_shutdown();
    return 0;
}

/********************************************************************
 * gpio_init()
 *
 *  Initialize the GPIO subsystems of BCM2835.
 * Initialize SPI bus, UART, and GPIO input/output pins.
 * Failure to initialize any of the above three IO subsystems
 * will result in closing all open IO devices and exiting with an error.
 *
 *  param:  none
 *  return: 0 if no error,
 *         -1 if error initializing any subcomponent or library
 *
 */
static int gpio_init(void)
{
    // try to initialize GPIO subsystem
    if ( !bcm2835_init() )
    {
        printf("         %s bcm2835_init failed. Are you running as root?\n", STATUS_FAIL);
        return -1;
    }

    printf("         %s Initialized GPIO\n", STATUS_OK);

    // Initialize RST GPIO pin
    bcm2835_gpio_fsel(LCD_RST, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(LCD_RST, HIGH);

    // Initialize SPI
    if (!bcm2835_spi_begin())
    {
        printf("         %s bcm2835_spi_begin failed. Are you running as root?\n", STATUS_FAIL);
        // Close GPIO
        bcm2835_close();

        return -1;
    }

    // Initialize SPI for the LCD according to wiring
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    printf("         %s Initialized SPI\n", STATUS_OK);

    // Reset the devices on the SPI bus
    bcm2835_gpio_write(LCD_RST, LOW);
    bcm2835_delay(250);
    bcm2835_gpio_write(LCD_RST, HIGH);

    // LCD initialization and test
    lcdInit();
    lcdSetRotation(LCD_ROTATION);
    lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);
    lcdFrameBufferPush(frame_buffer.pixel_bytes);

    vt100_lcd_init(LCD_ROTATION, 1, SYS_BG_COLOR, SYS_FG_COLOR);

    printf("         %s Initialized LCD\n", STATUS_OK);

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

    printf("         %s Initialized pushbutton IO pins\n", STATUS_OK);

    // Open UART0 port
    uart_fd = open(UART0, O_RDWR | O_NOCTTY | O_NDELAY);
    if ( uart_fd == -1 )
    {
        printf("         %s Error %d opening %s\n", STATUS_FAIL, errno, UART0);
        // Close SPI
        bcm2835_spi_end();
        // Close GPIO
        bcm2835_close();

        return -1;
    }
    else
    {
        // Setup UART options
        uart_set_interface_attr(uart_fd, B9600, 0);
        uart_set_blocking(uart_fd, 0);
        fcntl(uart_fd, F_SETFL, FNDELAY);

        printf("         %s Initialized UART0 %s\n", STATUS_OK, UART0);
    }

    return 0;
}

/********************************************************************
 * gpio_init()
 *
 *  Shutdown the GPIO subsystems in preparation for exiting.
 *
 *  param:  none
 *  return: none
 *
 */
static void gpio_shutdown(void)
{
    // Close SPI
    bcm2835_spi_end();
    // Close GPIO
    bcm2835_close();
    // Close the port and exit
    close(uart_fd);

    printf("         %s IO subsystems closed\n", STATUS_OK);
}

/********************************************************************
 * menu_print()
 *
 *  Print the main menu and high light an item.
 *
 *  param:  Item number to highlight
 *  return: none
 *
 */
static void menu_print(int highlight_item)
{
    int     i;
    int     row = 5;

    for ( i = MAIN_MENU_TOP; i <= MAIN_MENU_BOTTOM; i++, row++)
    {
        if ( i == highlight_item )
        {
            vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[%1d;2f%s%s%s", row, SYS_FONT_INV, menu_item[i], SYS_FONT_NORM);
        }
        else
        {
            vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[%1d;2f%s", row, menu_item[i]);
        }
    }
}

/********************************************************************
 * msg_not_implemented()
 *
 *  Print a "NOT IMPLEMENTED" message.
 *
 *  param:  none
 *  return: none
 *
 */
static void msg_not_implemented(void)
{
    lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);
    vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "%s%s", NOT_IMPLEMENTED, SYS_FONT_NORM);
    lcdFrameBufferPush(frame_buffer.pixel_bytes);
    bcm2835_delay(2000);
}

/********************************************************************
 * gps_data()
 *
 *  Read GPS NMEA data, parse, and print on screen.
 *  Exit back to main menu if "LEFT" button is pressed.
 *  This function serves a dual purpose, it can also log
 *  GPS location to a logger file for off-line plotting.
 *  If passed '0' argument it function as display only,
 *  if passed a non-'0' argument it also loggs data.
 *
 *  param:  '0' logger off, '1' logger on
 *  return: none
 *
 */
static void gps_data(int logger_on)
{
    int     time_invalid_fix = 0;
    char    heart_beat = '*';
    char    nmea_text[128] = {0};
    int     logger_fd = -1;
    int     logged_points = 0;

    int     read_result;
    int     valid_fix;

    // Format screen
    lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);
    vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[HPress 'LEFT' to exit.");

    // Initialize logger
    if ( logger_on && usb_mounted )
    {
        // Try to open logger file
        logger_fd = open(LOGGER_FILE, O_WRONLY | O_APPEND | O_CREAT);

        // Print once heading per logging session
        if ( logger_fd != -1 )
        {
            sprintf(nmea_text, "#\n# GPS logger\n#\n");
            write(logger_fd, nmea_text, strlen(nmea_text));
            sprintf(nmea_text, "#logged_points,gga_time,latitude,longitude,ground_spd,heading\n");
            write(logger_fd, nmea_text, strlen(nmea_text));
        }
    }

    // Error if cannot open logger
    if ( logger_on && (logger_fd == -1 || usb_mounted == 0) )
    {
        vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[11;0f\e[31;40m** Cannot open logger **%s", SYS_FONT_NORM);
    }

    // Flush stale NMEA data
    uart_flush(uart_fd);

    while ( push_button_read() != PB_LEFT)
    {
        // Try to read NMEA GPS text UART
        read_result = uart_read_line(uart_fd, nmea_text, 512);

        // If no new data don't proceed to update screen
        if ( read_result == 0 )
        {
            continue;
        }

        // If an error occurred, then abort
        else if ( read_result < 0 )
        {
            vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[10;0f\e[31;40mError %d on %s%s", errno, UART0, SYS_FONT_NORM);
        }

        // Only a valid NMEA text line can be present at this point
        else
        {
            vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[2;1f%c", heart_beat);
            heart_beat = (heart_beat == '*') ? ' ' : '*';
            valid_fix = nmea_update_pos(nmea_text, &pos);

            // *** Un-comment to fake a valid fix ***
            //valid_fix = 1;

            if ( valid_fix )
            {
                time_invalid_fix = 0;

                // Print position information
                // Move cursor, erase line, and reprint information
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[3;0f\e[2KUTC Time %02d:%02d:%#-6.3f", pos.hour, pos.min, pos.sec);
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[4;0f\e[2KLatitude %#-10.6f", pos.latitude);
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[5;0f\e[2KLongitude %#-10.6f", pos.longitude);
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[6;0f\e[2KSatellites %d", pos.sat_count);
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[7;0f\e[2KGround speed %-5.2f [mph]", pos.ground_spd);
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[8;0f\e[2KHeading %-5.1f [deg]", pos.heading);

                // Clear the error line just in case there was an alert
                vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[10;0f\e[2K");

                // log position point only if GGA and RMC data
                // are from the same NMEA message batch
                if ( logger_on && pos.gga_rmc_sync && logger_fd != -1)
                {
                    sprintf(nmea_text, "%d,%s,%#-10.6f,%#-10.6f,%-5.2f,%-5.1f\n", logged_points, pos.gga_time, pos.latitude, pos.longitude, pos.ground_spd, pos.heading);
                    write(logger_fd, nmea_text, strlen(nmea_text));
                    logged_points++;
                    vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[14;0f\e[2KLogged points: %-5d", logged_points);
                }
            }
            else
            {
                // There are six NMEA messages every second.
                // So if we count 60 invalid messages, we have an invalid fix for at least 10sec
                // then print the 'invalid fix' warning
                time_invalid_fix++;
                if ( time_invalid_fix > 60 )
                {
                    vt100_lcd_printf(frame_buffer.pixel_bytes, 0, "\e[10;0f\e[31;40m** Fix not valid **%s", SYS_FONT_NORM);
                }
            }
        }

        // Print the screen
        lcdFrameBufferPush(frame_buffer.pixel_bytes);
    }

    // Close logger file
    if ( logger_on && logger_fd != -1 )
    {
        close(logger_fd);
    }

}

/********************************************************************
 * gps_map_nav()
 *
 *  Read GPS NMEA data, parse, and print on screen.
 *  Exit back to main menu if "LEFT" button is pressed.
 *  This function serves a dual purpose, it can also log
 *  GPS location to a logger file for off-line plotting.
 *  If passed '0' argument it function as display only,
 *  if passed a non-'0' argument it also loggs data.
 *
 *  param:  none
 *  return: none
 *
 */
static void gps_map_nav(void)
{
    static struct map_t *loaded_map = NULL;

    char    nmea_text[128] = {0};
    char    heart_beat = '*';
    int     time_invalid_fix = 0;
    int     read_result;
    int     valid_fix;

    // Format screen
    lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);

    if ( map_list == NULL )
    {
        vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[11;0f\e[31;40m** No maps **%s", SYS_FONT_NORM);
    }

    // Flush stale NMEA data
    uart_flush(uart_fd);

    while ( push_button_read() != PB_LEFT)
    {
        // Try to read NMEA GPS text UART
        read_result = uart_read_line(uart_fd, nmea_text, 512);

        // If no new data don't proceed to update screen
        if ( read_result == 0 )
        {
            continue;
        }

        // If an error occurred, then abort
        else if ( read_result < 0 )
        {
            vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[10;0f\e[31;40mError %d on %s%s", errno, UART0, SYS_FONT_NORM);
        }

        // Only a valid NMEA text line can be present at this point
        else
        {
            valid_fix = nmea_update_pos(nmea_text, &pos);

#if  __FAKE_VALID_FIX__
            valid_fix = 1;
            pos.heading = 0.0;
            pos.latitude = 42.27216935370383;
            pos.longitude = -71.21417738855098;
#endif

            if ( valid_fix )
            {
                time_invalid_fix = 0;

                // If a map is already loaded, verify that it is still valid
                if ( loaded_map &&
                     pos.latitude <= loaded_map->tl_lat && pos.latitude >= loaded_map->br_lat &&
                     pos.longitude >= loaded_map->tl_long && pos.longitude <= loaded_map->br_long )
                {
                    // Current map is still valid, so load patch into screen buffer
/*
                    if ( map_image == NULL )
                        map_image = load_map_image(loaded_map);
*/
                    get_map_patch(&pos, loaded_map, map_image);
                }

                // Otherwise find a map to load
                else
                {
                    // Scan the linked list for an appropriate map
                    // that contains the current location
                    for ( loaded_map = map_list; loaded_map; loaded_map = loaded_map->next )
                    {
                        if ( pos.latitude <= loaded_map->tl_lat && pos.latitude >= loaded_map->br_lat &&
                             pos.longitude >= loaded_map->tl_long && pos.longitude <= loaded_map->br_long )
                        {
                            break;
                        }
                    }

                    // Reload the new map and render a patch or output an error notification
                    if ( loaded_map )
                    {
                        map_image = load_map_image(loaded_map);
                        get_map_patch(&pos, loaded_map, map_image);
                    }
                    else
                    {
                        lcdFrameBufferColor(frame_buffer.pixel_bytes, SYS_BG_COLOR);
                        vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[12;0f\e[31;40m** No map for location **%s", SYS_FONT_NORM);
                    }
                }

                lcdDrawChar(frame_buffer.pixel_bytes, 78, 60, 0, ST7735_BLUE, ST7735_BLACK, 1, 1);
            }
            else
            {
                // There are six NMEA messages every second.
                // So if we count 60 invalid messages, we have an invalid fix for at least 10sec
                // then print the 'invalid fix' warning
                time_invalid_fix++;
                if ( time_invalid_fix > 60 )
                {
                    vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[13;0f\e[31;40m** Fix not valid **%s", SYS_FONT_NORM);
                }
            }
        }

        // Print the screen
        vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[15;0f\e[34;40mPress 'LEFT' to exit.%s", SYS_FONT_NORM);

        heart_beat = (heart_beat == '*') ? ' ' : '*';
        vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[0;0f\e[34;40m%c%s", heart_beat, SYS_FONT_NORM);

        lcdFrameBufferPush(frame_buffer.pixel_bytes);
    }

    // Invalidate the map image buffer and exit
    free(map_image);
    map_image = NULL;
}

/********************************************************************
 * load_map_image()
 *
 *  Load the map image referenced by the map meta date
 *  structure in loaded_map.
 *  This function uses 'realloc' to allocate memory for the map image,
 *  and the calling application should free this buffer.
 *
 *  param:  Pointer to current map meta data
 *  return: Pointer to allocated buffer containing map image pixels
 *
 */
uint16_t *load_map_image(struct map_t *loaded_map)
{
    uint16_t   *image_buffer;
    size_t      image_size;
    char        raw_img_file[128] = {USB_DIR};
    int         fd;

    // Allocate a buffer for the image that is pixel count of uint16_t
    image_size = sizeof(uint16_t) * loaded_map->height * loaded_map->width;
    image_buffer = realloc(map_image, image_size);

    // If allocation is of, load the image pixel data from file
    if ( image_buffer )
    {
        // Setup directory and file name string,
        // then open the file
        strncat(raw_img_file, "/", MAX_FILE_NAME_LEN);
        strncat(raw_img_file, loaded_map->file_name, MAX_FILE_NAME_LEN);
        fd = open(raw_img_file, O_RDONLY);
        if ( fd == -1 )
        {
            free(image_buffer); // Error checking of the image buffer pointer
            return NULL;        //  will be done in get_map_patch()
        }

        // Read the file content into the buffer
        read(fd, (void *)image_buffer, image_size);
    }

    return image_buffer;
}

/********************************************************************
 * get_map_patch()
 *
 *  Load a map patch from the map image buffer into the screen buffer.
 *  The map patch is rotated according to the current heading.
 *
 *  param:  Pointer to current pos data, pointer to loaded map meta data, pointer to map image buffer
 *  return: None. Screen buffer will contain map patch
 *
 */
static void get_map_patch(struct position_t *pos, struct map_t *map_attrib, uint16_t *image_buffer)
{
    int     theta, y, x, yt, xt, u, v;
    int     roi_img_height, roi_img_width;
    int     hwidth, hheight;
    int     roi_center_x, roi_center_y;
    int     roi_index, img_index;
    double  map_res_x, map_res_y;

    // Sanity check
    if ( image_buffer == NULL )
    {
        lcdFrameBufferColor(frame_buffer.pixel_bytes, ST7735_BLACK);
        vt100_lcd_printf(frame_buffer.pixel_bytes, 1, "\e[8;0f\e[31;40m** Map load error\n   image_buffer == NULL **%s", SYS_FONT_NORM);
    }

    // Initialize variables for calculation
    theta = (int)pos->heading;
    roi_img_height = lcdHeight();
    roi_img_width = lcdWidth();
    hheight = roi_img_height / 2;
    hwidth = roi_img_width / 2;

    // Calculate the center of the display in pixels based on current position
    map_res_x = fabs(map_attrib->br_long - map_attrib->tl_long) / (double)map_attrib->width;
    roi_center_x = (int)(fabs(pos->longitude - map_attrib->tl_long) / map_res_x);

    map_res_y = fabs(map_attrib->br_lat - map_attrib->tl_lat) / (double)map_attrib->height;
    roi_center_y = (int)(fabs(pos->latitude - map_attrib->tl_lat) / map_res_y);

/*
    printf("lat %lf, long %lf\n", map_attrib->tl_lat, map_attrib->tl_long);
    printf("lat %lf, long %lf\n", map_attrib->br_lat, map_attrib->br_long);
    printf("lat %lf, long %lf\n", pos->latitude, pos->longitude);
    printf("res_x %lf, res_y %lf\n", map_res_x*1000, map_res_y*1000);
    printf("roi_x %d, roi_y %d\n\n", roi_center_x, roi_center_y);
*/

    // Copy rotated map patch from map image to display buffer
    for ( y = 0; y < roi_img_height; y++ )
    {
        for ( x = 0; x < roi_img_width; x++ )
        {
            xt = x - hwidth;
            yt = y - hheight;

            u = (int)(xt * COS[theta] - yt * SIN[theta]) + roi_center_x;
            v = (int)(xt * SIN[theta] + yt * COS[theta]) + roi_center_y;

            roi_index = (y * roi_img_width) + x;

            if ( u >= 0 && u < map_attrib->width && v >= 0 && v < map_attrib->height )
            {
                img_index = (v * map_attrib->width) + u;
                frame_buffer.pixel_words[roi_index] = image_buffer[img_index];
            }
            else
                frame_buffer.pixel_words[roi_index] = ST7735_BLACK;
        }
    }
}
