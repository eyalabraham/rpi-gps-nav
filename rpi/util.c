/********************************************************************
 * util.c
 *
 *  Various utilities for use across the modules.
 *
 *  March 16, 2018
 *
 *******************************************************************/

#define     _GNU_SOURCE

#include    <stdlib.h>
#include    <stdio.h>
#include    <errno.h>
#include    <fcntl.h>
#include    <string.h>
#include    <termios.h>
#include    <unistd.h>
#include    <ctype.h>
#include    <libxml/parser.h>
#include    <libxml/tree.h>

#include    "util.h"
#include    "config.h"

/********************************************************************
 * Module definitions
 *
 */
#define     PB_DEBUONCE     100     // push button debounce delay in mSec

/********************************************************************
 * Static functions
 *
 */
static int  get_maps(xmlNode*, struct map_t**);
static void get_map_elements(xmlNode *, struct map_t *);

/********************************************************************
 * uart_set_interface_attr()
 *
 *  Initialize UART attributes.
 *
 *  param:  file descriptor, baud rate, and parity type (to enable: PARENB + for odd: PARODD)
 *  return: 0 if no error,
 *         -1 if error initializing
 *
 */
int uart_set_interface_attr(int fd, int speed, int parity)
{
    struct termios tty;

    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("         Error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf("         Error %d from tcsetattr", errno);
        return -1;
    }

    return 0;
}

/********************************************************************
 * uart_set_blocking()
 *
 *  Initialize UART attributes.
 *
 *  param:  file descriptor, '0' non-blocking read or '1' blocking read
 *  return: 0 if no error,
 *         -1 if error initializing
 *
 */
int uart_set_blocking(int fd, int should_block)
{
    struct termios tty;

    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("         Error %d from tggetattr", errno);
        return -1;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf("         Error %d setting term attributes", errno);
        return -1;
    }

    return 0;
}

/********************************************************************
 * uart_read_line()
 *
 *  Read a text line from the UART stream.
 *  Reads an entire line from file 'fd', storing characters read into a buffer
 *  pointed to by 'lineptr'.  The buffer is null-terminated
 *  and does not includes the newline character, if one was found before
 *  storage buffer space is depleted.
 *  Before calling uart_read_line(), *lineptr should contain a
 *  pointer to an allocated buffer *n bytes in size.  If the
 *  buffer is not large enough to hold the line, only *n characters will
 *  be read and null terminated, and the function will return the numbers of
 *  characters read.
 *
 *  param:  file descriptor, pointer to input buffer, size of input buffer
 *  return: Number of characters read including the delimiter character, but not including the terminating null byte ('\0')
 *         -1 if error initializing
 *
 */
int uart_read_line(int fd, char *lineptr, int n)
{
    char   *str;
    char    c;
    int     read_result;
    int     count = 0;

    if ( lineptr == NULL || n == 0 )
        return 0;

    str = lineptr;

    // Read c character from UART and pack into the destination buffer
    // passed in lineptr. Terminate with '\0' when buffer space is depleted or
    // a new line character is encountered.
    while ( count < (n - 1) )
    {
        read_result = read(fd, &c, 1);

        // handle errors in read
        if ( read_result == -1 && errno == EAGAIN )
        {
            continue;
        }

        else if ( read_result == -1 && errno != EAGAIN )
        {
            return -1;
        }

        // nothing read so keep looping for next character
        // TODO need timeout here
        else if ( read_result == 0 )
        {
            break;
        }

        // end of line so terminate string and exit
        else if ( c == '\n' || c == '\r' )
        {
            *str = '\0';
            break;
        }

        *str++ = c;
        count++;
    }

    return count;
}

/********************************************************************
 * uart_flush()
 *
 *  Flush UART buffer.
 *
 *  param:  file descriptor
 *  return: '0' on success. '-1' on failure and set errno to indicate the error
 *
 */
int uart_flush(int fd)
{
    return tcflush(fd, TCIOFLUSH);
}

/********************************************************************
 * lstrip()
 *
 *  Return the input string with leading characters removed.
 *  If 'delim' is NULL, whitespace characters are removed (tab, space, newline etc. as
 *  defined by isspace() function).
 *  If 'delim' is given and not NULL, chars must be a string; the characters in the
 *  string will be stripped from the beginning of the 'str' this function is called on.
 *
 *  param:  character string to strip, character string of characters to search and strip from 'str'
 *  return: Pointer to '\0' terminated string with 'delim' characters stripped from its left side
 *          NULL if error
 *
 */
char *lstrip(char *str, char *delim)
{
    int     delim_test = 0;
    int     done = 0;
    char   *tmp;
    char    c;

    // Safety check
    if ( str == NULL )
        return NULL;

    // Set a flag for testing against provided delimiter or
    // default white space characters
    if ( delim )
        delim_test = 1;
    else
        delim_test = 0;

    tmp = str;
    tmp++;

    while ( (c = *str) != '\0' && !done )
    {
        // test character against the list of delimiter characters
        // stop processing on encounter of first character that is not a delimiter
        if ( delim_test )
        {
            if ( strchr(delim, c) )
                // Copy/move 'tmp' into 'str' including the terminating '\0'
                // so strlen(str) or strlen(tmp)+1 bytes
                memmove(str, tmp, strlen(str));
            else
                done = 1;
        }

        // test the character against default white space
        // stop processing on encounter of first character that is not a delimiter
        else
        {
            if ( isspace(c) )
                memmove(str, tmp, strlen(str));
            else
                done = 1;
        }
    }

    return str;
}

/********************************************************************
 * rstrip()
 *
 *  Return the input string with trailing characters removed.
 *  If 'delim' is NULL, whitespace characters are removed (tab, space, newline etc. as
 *  defined by isspace() function).
 *  If 'delim' is given and not NULL, chars must be a string; the characters in the
 *  string will be stripped from the end of the 'str' this function is called on.
 *
 *  param:  character string to strip, character string of characters to search and strip from 'str'
 *  return: Pointer to '\0' terminated string with 'delim' characters stripped from its right side
 *          NULL if error
 *
 */
char *rstrip(char *str, char *delim)
{
    int     delim_test = 0;
    int     done = 0;
    char    c;
    int     char_index;

    // Safety check
    if ( str == NULL )
        return NULL;

    // Set a flag for testing against provided delimiter or
    // default white space characters
    if ( delim )
        delim_test = 1;
    else
        delim_test = 0;

    char_index = strlen(str)-1;

    while ( char_index && !done )
    {
        c = str[char_index];

        // test character against the list of delimiter characters
        // stop processing on encounter of first character that is not a delimiter
        if ( delim_test )
        {
            if ( strchr(delim, c) )
                str[char_index] = '\0';
            else
                done = 1;
        }

        // test the character against default white space
        // stop processing on encounter of first character that is not a delimiter
        else
        {
            if ( isspace(c) )
                str[char_index] = '\0';
            else
                done = 1;
        }

        char_index--;
    }

    return str;
}

/********************************************************************
 * nmea_get_field()
 *
 *  Extract and return an NMEA field as a string.
 *  Divide the NMEA sentence by the 'delim' character, and return a '\0' terminated
 *  string containing the requested 'field'. Field index 'field' is 0 based.
 *  The function returns a pointer to the field string, or NULL if the field was not found.
 *  The returned string can be of zero length if the field is empty.
 *
 *  param:  NMEA sentence character string,
 *          0-based field index,
 *          delimiter character,
 *          pointer to allocated field string and its length
 *  return: Field length in bytes, or '-1' if error
 *
 */
int nmea_get_field(const char *nmea_str, char delim, int field, char *field_str, int field_size)
{
    char   *str;
    int     field_len;

    // Safety check
    if ( field < 0 || nmea_str == NULL || field_str == NULL || field_size == 0 )
        return -1;

    // try to find the first occurrence of the field delimiter
    // strchrnul() will point to the string terminating '\0' if not found
    str = strchrnul(nmea_str, delim);
    field_len = str - nmea_str;

    // iterate over fields to find the field we want to extract
    while ( field && *str)
    {
        nmea_str = str + 1;
        str = strchrnul(nmea_str, delim);
        field_len = str - nmea_str;
        field--;
    }

    // this test means that the delimiter does
    // not divide the string into enough fields
    if ( field != 0 )
        return -1;

    // Copy the field into the return string space
    field_len = ((field_len+1) > field_size) ? field_size : field_len;
    memset(field_str, 0, field_len + 1);
    memcpy(field_str, nmea_str, field_len);
    field_str[field_len] = '\0';

    return field_len;
}

/********************************************************************
 * nmea_checksum()
 *
 *  Calculate checksum over an NMEA sentence passed as a string.
 *  The NMEA sentence should not contain the '$' start character,
 *  the '*' checksum delimiter and the checksum characters.
 *
 *  param:  NMEA sentence string
 *  return: Calculated checksum
 *
 */
int nmea_checksum(char *str)
{
    uint8_t     checksum = 0;

    while ( *str )
    {
        checksum ^= (uint8_t)(*str++);
    }

    return (int)checksum;
}

/********************************************************************
 * nmea_update_pos()
 *
 *  Extract GPS information from NMEA sentence string,
 *  and update GPS position data structure.
 *  Function handles only 'GGA' and 'RMC' sentences.
 *
 *  param:  NMEA sentence string, pointer to position data structure
 *  return: 1- valid fix indicated, 0- Invalid fix indicated
 *
 */
int nmea_update_pos(char *str, struct position_t *pos)
{
    int     exit_value = 0;
    char    gps_data[128] = {0};
    char    checksum_str[4] = {0};
    char    data_field[16] = {0};
    float   deg, min;

    // Strip '$' header and separate the fields in the NMEA sentence
    lstrip(str, "$");
    nmea_get_field(str, '*', NMEA_MSG, gps_data, 128);
    nmea_get_field(str, '*', NMEA_CHECKSUM, checksum_str, 4);

    // Validate the checksum
    if ( strtol(checksum_str, NULL, 16) != nmea_checksum(gps_data) )
    {
        return 0;
    }

    // Handle GGA and RMC sentences
    nmea_get_field(gps_data, ',', NMEA_MSG_ID, data_field, 16);

    if ( strcmp(data_field, "GPGGA") == 0 )
    {
        nmea_get_field(gps_data, ',', NMEA_GGA_FIXOK, data_field, 16);
        if ( atoi(data_field) == 1 )
        {
            nmea_get_field(gps_data, ',', NMEA_GGA_UTC, data_field, 16);
            strncpy(pos->gga_time, data_field, 16);
            sscanf(data_field, "%2d%2d%6f", &(pos->hour), &(pos->min), &(pos->sec));

            nmea_get_field(gps_data, ',', NMEA_GGA_LAT, data_field, 16);
            sscanf(data_field, "%2f%7f", &deg, &min);
            pos->latitude = deg + min/60.0;
            nmea_get_field(gps_data, ',', NMEA_GGA_NS, data_field, 16);
            if ( data_field[0] == 'S')
                pos->latitude *= -1.0;

            nmea_get_field(gps_data, ',', NMEA_GGA_LONG, data_field, 16);
            sscanf(data_field, "%3f%7f", &deg, &min);
            pos->longitude = deg + min/60.0;
            nmea_get_field(gps_data, ',', NMEA_GGA_EW, data_field, 16);
            if ( data_field[0] == 'W')
                pos->longitude *= -1.0;

            nmea_get_field(gps_data, ',', NMEA_GGA_SAT, data_field, 16);
            pos->sat_count = atoi(data_field);

            exit_value = 1;
        }
        else
            exit_value = 0;
    }
    else if ( strcmp(data_field, "GPRMC") == 0 )
    {
        nmea_get_field(gps_data, ',', NMEA_RMC_STATUS, data_field, 16);
        if ( strcmp(data_field, "A") == 0 )
        {
            nmea_get_field(gps_data, ',', NMEA_RMC_UTC, data_field, 16);
            strncpy(pos->rmc_time, data_field, 16);

            nmea_get_field(gps_data, ',', NMEA_RMC_GNDSPD, data_field, 16);
            sscanf(data_field, "%4f", &(pos->ground_spd));
            pos->ground_spd *= 1.150779;

            nmea_get_field(gps_data, ',', NMEA_RMC_COURSE, data_field, 16);
            sscanf(data_field, "%6f", &(pos->heading));

            exit_value = 1;
        }
        else
            exit_value = 0;
    }

    // When both GGA and RMC data come from the same batch of NMEA
    // messages they position data structure is in sync
    // The condition here will only be true after a GGA and then an RMC message
    // are decoded without error
    if ( exit_value == 1 && strcmp(pos->gga_time, pos->rmc_time) == 0 )
        pos->gga_rmc_sync = 1;
    else
        pos->gga_rmc_sync = 0;

    return exit_value;
}

/********************************************************************
 * push_button_read()
 *
 *  Read push button state and return code of button pressed.
 *
 *  param:  none
 *  return: push button code, value 0 through 4 or '-1' if none pressed
 *
 */
int push_button_read(void)
{
    int     push_button_code = -1;

    // Read and de-bounce UP button
    if ( bcm2835_gpio_lev(PBUTTON_UP) == LOW )
    {
        bcm2835_delay(PB_DEBUONCE);
        if ( bcm2835_gpio_lev(PBUTTON_UP) == LOW )
        {
            push_button_code = PB_UP;
        }
    }

    // Read and de-bounce UP button
    else if ( bcm2835_gpio_lev(PBUTTON_DOWN) == LOW )
    {
        bcm2835_delay(PB_DEBUONCE);
        if ( bcm2835_gpio_lev(PBUTTON_DOWN) == LOW )
        {
            push_button_code = PB_DOWN;
        }
    }

    // Read and de-bounce LEFT button
    else if ( bcm2835_gpio_lev(PBUTTON_LEFT) == LOW )
    {
        bcm2835_delay(PB_DEBUONCE);
        if ( bcm2835_gpio_lev(PBUTTON_LEFT) == LOW )
        {
            push_button_code = PB_LEFT;
        }
    }

    // Read and de-bounce RIGHT button
    else if ( bcm2835_gpio_lev(PBUTTON_RIGHT) == LOW )
    {
        bcm2835_delay(PB_DEBUONCE);
        if ( bcm2835_gpio_lev(PBUTTON_RIGHT) == LOW )
        {
            push_button_code = PB_RIGHT;
        }
    }

    // Read and de-bounce SELECT button
    else if ( bcm2835_gpio_lev(PBUTTON_SELECT) == LOW )
    {
        bcm2835_delay(PB_DEBUONCE);
        if ( bcm2835_gpio_lev(PBUTTON_SELECT) == LOW )
        {
            push_button_code = PB_SELECT;
        }
    }

    return push_button_code;
}

/********************************************************************
 * get_maps()
 *
 *  Recursive parsing of a maps XML to find elements and build
 *  map linked list data structure.
 *  Source: http://hamburgsteak.sandwich.net/writ/libxml2.txt
 *
 *  param:  the initial xml map node to consider, address of pointer to map inked list
 *  return: number of maps in the list, '-1' if parsing error
 *
 */
static int get_maps(xmlNode *a_node, struct map_t **map_meta_data_ptr)
{
    int      map_count = 0;
    xmlNode *cur_node = NULL;
    struct map_t *new_map_meta_data;
    struct map_t *curr_map;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
        if ( cur_node->type == XML_ELEMENT_NODE && xmlStrEqual(cur_node->name, (const xmlChar *)"map") )
        {
            // try to allocate a data structure for a new map
            new_map_meta_data = malloc(sizeof(struct map_t));
            if ( new_map_meta_data == NULL )
                return -1;

            memset(new_map_meta_data, 0, sizeof(struct map_t));

            // link the new map to the head of list pointer if the head is NULL
            if ( *map_meta_data_ptr == NULL )
            {
                *map_meta_data_ptr = new_map_meta_data;
            }
            // otherwise link to previous map
            else
            {
                curr_map->next = new_map_meta_data;
            }

            // set current map and populate with meta data elements
            curr_map = new_map_meta_data;
            get_map_elements(cur_node->children, curr_map);
            map_count++;
        }
    }

    return map_count;
}

/********************************************************************
 * get_map_elements()
 *
 *  Recursive parsing of a maps XML to find elements and build
 *  map linked list data structure.
 *  Source: http://hamburgsteak.sandwich.net/writ/libxml2.txt
 *
 *  param:  the initial xml map node to consider, address of pointer to map inked list
 *  return: none
 *
 */
static void get_map_elements(xmlNode *a_node, struct map_t *map_meta_data)
{
    xmlNode *cur_node = NULL;
    xmlChar *content;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
        if ( cur_node->type == XML_ELEMENT_NODE && xmlStrEqual(cur_node->name, (const xmlChar *)"file") )
        {
            content = xmlNodeGetContent(cur_node);
            strncpy(map_meta_data->file_name, (const char *)content, MAX_FILE_NAME_LEN);
            xmlFree(content);
        }
        else if ( cur_node->type == XML_ELEMENT_NODE && xmlStrEqual(cur_node->name, (const xmlChar *)"height") )
        {
            content = xmlNodeGetContent(cur_node);
            map_meta_data->height = atoi((const char *)content);
            xmlFree(content);
        }
        else if ( cur_node->type == XML_ELEMENT_NODE && xmlStrEqual(cur_node->name, (const xmlChar *)"width") )
        {
            content = xmlNodeGetContent(cur_node);
            map_meta_data->width = atoi((const char *)content);
            xmlFree(content);
        }
        else if ( cur_node->type == XML_ELEMENT_NODE && xmlStrEqual(cur_node->name, (const xmlChar *)"top_left") )
        {
            content = xmlGetProp(cur_node, (const xmlChar *)"latitude");
            sscanf((const char *)content, "%lf", &(map_meta_data->tl_lat));
            xmlFree(content);
            content = xmlGetProp(cur_node, (const xmlChar *)"longitude");
            sscanf((const char *)content, "%lf", &(map_meta_data->tl_long));
            xmlFree(content);
        }
        else if ( cur_node->type == XML_ELEMENT_NODE && xmlStrEqual(cur_node->name, (const xmlChar *)"bottom_right") )
        {
            content = xmlGetProp(cur_node, (const xmlChar *)"latitude");
            sscanf((const char *)content, "%lf", &(map_meta_data->br_lat));
            xmlFree(content);
            content = xmlGetProp(cur_node, (const xmlChar *)"longitude");
            sscanf((const char *)content, "%lf", &(map_meta_data->br_long));
            xmlFree(content);
        }
    }
}

/********************************************************************
 * new_map_list()
 *
 *  Read map XML file and create a linked list of map data
 *  structures. Return NULL is failed to find/open XML file
 *  but will return list of first N valid maps from the file.
 *  Uses malloc() to allocated linked list, use del_map_list() to
 *  delete the list and free memory
 *  Source: http://hamburgsteak.sandwich.net/writ/libxml2.txt
 *
 *  param:  map meta data XML file name, address of pointer to map inked list
 *  return: number of maps in the list, '-1' if parsing error
 *
 */
int new_map_list(const char *filename, struct map_t **map_linked_list)
{
    xmlDoc  *doc = NULL;
    xmlNode *root_element = NULL;
    int      map_count = 0;

    /* this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    // parse the file and get the DOM
    doc = xmlParseFile(filename);

    if (doc == NULL)
    {
        return -1;
    }

    // Get the root element node
    root_element = xmlDocGetRootElement(doc);
    if ( xmlStrEqual(root_element->name, (const xmlChar *)"maps") )
    {
        map_count = get_maps(root_element->children, map_linked_list);
    }
    else
    {
        printf("Error, this is not a 'maps' XML file.\n");
    }

    // free the document
    xmlFreeDoc(doc);

    /* Free the global variables that may
     * have been allocated by the parser.
     */
    xmlCleanupParser();

    return map_count;
}

/********************************************************************
 * del_map_list()
 *
 *  Free map linked list allocated with new_map_list().
 *
 *  param:  pointer to linked list of file data structures
 *  return: none
 *
 */
void del_map_list(struct map_t *map_linked_list)
{
    struct map_t    *map_ptr;
    struct map_t    *next_map_ptr;

    next_map_ptr = map_linked_list;
    map_ptr = map_linked_list;

    // walk down the linked list freeing memory
    while ( next_map_ptr )
    {
        next_map_ptr = map_ptr->next;
        free(map_ptr);
        map_ptr = next_map_ptr;
    }
}

/********************************************************************
 * dump_map_list()
 *
 *  Print map linked list to stdout.
 *
 *  param:  pointer to linked list of file data structures
 *  return: none
 *
 */
void dump_map_list(struct map_t *map_linked_list)
{
    struct map_t    *map_ptr;

    if ( map_linked_list )
    {
        for ( map_ptr = map_linked_list; map_ptr; map_ptr = map_ptr->next )
        {
            printf("                file: %s\n", map_ptr->file_name);
            printf("                  pixels: %d x %d\n", map_ptr->width, map_ptr->height);
            printf("                  top left: %f, %f\n", map_ptr->tl_lat, map_ptr->tl_long);
            printf("                  bottom right: %f, %f\n", map_ptr->br_lat, map_ptr->br_long);
        }
    }
    else
        printf("No maps.\n");
}
