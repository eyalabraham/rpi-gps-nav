/********************************************************************
 * util.h
 *
 *  Header file for utility functions.
 *
 *  March 16, 2018
 *
 *******************************************************************/


#ifndef __util_h__
#define __util_h__

/********************************************************************
 * Global definitions
 *
 */

// NMEA sentence (message) field indexes
#define     NMEA_MSG        0
#define     NMEA_CHECKSUM   1

#define     NMEA_MSG_ID     0

#define     NMEA_GGA_ID     NMEA_MSG_ID
#define     NMEA_GGA_UTC    1
#define     NMEA_GGA_LAT    2
#define     NMEA_GGA_NS     3
#define     NMEA_GGA_LONG   4
#define     NMEA_GGA_EW     5
#define     NMEA_GGA_FIXOK  6
#define     NMEA_GGA_SAT    7
#define     NMEA_GGA_HDOP   8
#define     NMEA_GGA_ALT    9
#define     NMEA_GGA_ALTU   10
#define     NMEA_GGA_GEOID  11
#define     NMEA_GGA_GEOIDU 12
#define     NMEA_GGA_DC     13
#define     NMEA_GGA_DCID   14

#define     NMEA_RMC_ID     NMEA_MSG_ID
#define     NMEA_RMC_UTC    1
#define     NMEA_RMC_STATUS 2
#define     NMEA_RMC_LAT    3
#define     NMEA_RMC_NS     4
#define     NMEA_RMC_LONG   5
#define     NMEA_RMC_EW     6
#define     NMEA_RMC_GNDSPD 7
#define     NMEA_RMC_COURSE 8
#define     NMEA_RMC_DATE   9
#define     NMEA_RMC_MAGVAR 10
#define     NMEA_RMC_VARSNS 11
#define     NMEA_RMC_MODE   12

// Push button codes
#define     PB_NONE        -1
#define     PB_SELECT       0
#define     PB_UP           1
#define     PB_DOWN         2
#define     PB_LEFT         3
#define     PB_RIGHT        4

/********************************************************************
 * Type definitions
 *
 */
struct position_t
{
    int     gga_rmc_sync;
    char    gga_time[16];
    char    rmc_time[16];
    int     hour;
    int     min;
    float   sec;
    int     sat_count;
    double  latitude;
    double  longitude;
    float   ground_spd;
    float   heading;
};

#define     MAX_FILE_NAME_LEN   32
struct map_t
{
    char    file_name[MAX_FILE_NAME_LEN];
    int     height;
    int     width;
    double  tl_lat;
    double  tl_long;
    double  br_lat;
    double  br_long;
    struct map_t *next;
};

/********************************************************************
 * Function prototypes
 *
 */

// UART functions
int   uart_set_interface_attr(int, int, int);
int   uart_set_blocking(int, int);
int   uart_read_line(int, char *, int);
int   uart_flush(int);

// String functions
char* lstrip(char *, char *);
char* rstrip(char *, char *);

// NMEA sentence parsing
int   nmea_get_field(const char *, char, int, char *, int);
int   nmea_checksum(char *);
int   nmea_update_pos(char *, struct position_t *);

// Push button read
int   push_button_read(void);

// Map functions
int   new_map_list(const char *, struct map_t **);
void  del_map_list(struct map_t *);
void  dump_map_list(struct map_t *);

#endif  /* __util_h__ */
