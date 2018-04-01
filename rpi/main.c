/********************************************************************
 * main.c
 *
 *  Main module for map based GPS navigation application
 *  for Raspberry Pi Model B.
 *  Required bcm2836 GPIO library from http://www.airspayce.com/mikem/bcm2835/index.html
 *  and tested with release 1.55 of this library on RPi Model B.
 *  Requires GPS module (serial UART) and 1.8" TFT LCD display (SPI).
 *
 *  Usage:
 *      navigator [ -t <test_num> ]
 *
 *  March 15, 2018
 *
 *******************************************************************/

#include    <ctype.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>

#include    "test.h"
#include    "nav.h"

/********************************************************************
 * main()
 *
 * return: 0 if ok
 *         1 if any errors
 */
int main (int argc, char **argv)
{
    char    *test_num_str = NULL;
    int     c;
    int     test_code;
    int     return_code = 0;

    // Process command line to extract test number
    opterr = 0;
    while ((c = getopt (argc, argv, "t:")) != -1)
    {
        switch (c)
        {
            case 't':
                test_num_str = optarg;
                break;

            case '?':
                if (optopt == 't')
                    printf ("Option -%c requires a test number to execute.\n", optopt);
                else if (isprint (optopt))
                    printf ("Unknown option `-%c'.\n", optopt);
                else
                    printf ("Unknown option character `\\x%x'.\n", optopt);
                return -1;

            default:
                exit(-1);
        }
    }

    // If test number was supplied on command line
    // convert to integer and attempt to execute test before exiting.
    if ( test_num_str )
    {
        test_code = atoi(test_num_str);
        switch ( test_code )
        {
            case 0:
                return_code = test_t0_lcd();
                break;
                
            case 1:
                return_code = test_t1_pbuttons();
                break;

            case 2:
                return_code = test_t2_gps();
                break;

            default:
                printf("Unrecognized test code %d\n", test_code);
                return_code = 1;
        }
        
        printf("Test %d %s\n", test_code, return_code ? "FAIL" : "PASS");
    }
    
    // If no test number was supplied then start the GPS
    // navigation module
    else
    {
        return_code = navigator();
    }

    return return_code;
}

