/********************************************************************
 * config.h
 *
 *  Header file that defines the hardware configuration and GPIO
 *  pin connectivity/function.
 *
 *  March 18, 2018
 *
 *******************************************************************/

#ifndef __config_h__
#define __config_h__

#include    <bcm2835.h>

/********************************************************************
 * GPIO pin functionality
 *
 */
#define     LCD_ROTATION    3
#define     LCD_DATA_CMD    RPI_V2_GPIO_P1_11
#define     LCD_RST         RPI_V2_GPIO_P1_15

#define     PBUTTON_UP      RPI_V2_GPIO_P1_13
#define     PBUTTON_DOWN    RPI_V2_GPIO_P1_12
#define     PBUTTON_LEFT    RPI_V2_GPIO_P1_05
#define     PBUTTON_RIGHT   RPI_V2_GPIO_P1_03
#define     PBUTTON_SELECT  RPI_V2_GPIO_P1_07

#define     UART0           "/dev/ttyAMA0"      // 9600, 8N1

#endif  /* __config_h__ */
