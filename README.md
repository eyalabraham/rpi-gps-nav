# Portable (solar powered?) GPS
I have a GPS shield that I got a couple of years ago from friends as a gift. The shield was tucked away in one of my parts drawers waiting for the right moment to be used in a project. I'm not sure why I decided to build this project because a hand held GPS is really quite useless these days. Maybe because I had all the components and I wanted to put the GPS module to use.
The design is basic revolves around five components: a GPS module, a Raspberry Pi, an 1.8" TFT LCD screen, and some push buttons for control. The goal is to build a stand alone, hand held, GPS device that will display its location on a moving map.
The device works by using swaths of Google map screen shots stored on the SD card, along with an XML file that describes the map's boundary coordinates. GPS readouts are used to pick the proper map swath, and center a portion of the map to be displayed on the LCD.
The device is battery powered with (maybe) the aid of solar cells to help charge the batteries during daylight.
## Software
### Block diagram of the software modules

    +-------------------------------------------+
    |                                           |
    |               main.c                      |
    |                 |                         |
    |                 +-- test.c                |
    |                 |                         |
    |                 +-- nav.c                 |
    |                      |                    |
    |                      +-- util.c           |
    |                                           |
    +--------+------------+------+--------------+ 
    |        |            |      |              |
    |        | vt100lcd.c |      |              |
    |        |            |      |              |
    |        +------------+      |              |
    |                     |      |              |
    |      pilcd.c        |      |              |
    |                     |      |              |
    +---------------------+      |              |
    |                            |              |
    |        libbcm2835.o        |  OS Serial   |
    |                            |  driver      |
    |                            |              |
    +----------------------------+--------------+

See file descriptions below.

### Block diagram of corresponding devices

    +------------+--------------+--------------+
    |            :              :              |
    |    SPI     :     GPIO     :     UART     |
    |            :              :              |
    +------------+--------------+--------------+
    |            |              |              |
    |     LCD    | Push buttons |     GPS      |
    |            |              |              |
    +------------+--------------+--------------+

### Drivers
* For GPIO and SPI using libbmc2835 [http://www.airspayce.com/mikem/bcm2835/] and its Python binding [https://github.com/mubeta06/py-libbcm2835]
* For UART serial communication with GPS module using Raspbian built in serial driver
### tbd
### Project files
- *main.c* Main module
- *test.c* Contains various test routines activated by optional command line switch -t <num>
- *nav.c* Main GPS and man navigation application.
- *util.c* Processing utilities, XML parsing, NMEA sentence parsing and coordinate conversions etc
- *pilcd.c* TFT LCD driver (ST7735 device) including text and graphics functions.
- *vt100lcd.c* VT100-aware prinf functions for LCD
- *libbcm2835.so* BCM2835 GPIO driver from [http://www.airspayce.com/mikem/bcm2835/index.html]
- *mapcoord.py* a Python test program that loads a map image and helped me validate the translation between GPS coordinates and pixels on an image of a map. This program uses OpenCV and a screen capture of a map. Due to the curvature of the earth, the map swaths need to be around 2 to 3 square miles. I did not test where the linearity of the mapping breaks.
- *imgconvert.py* This program takes in a map image in any format that OpenCV understand (screen capture) and its GPS coordinates, and converts the image to an LCD formatter RGB565 raw pixel data. The program updates a maps XML database file with the converted file's map attributes.
- *showrawimg.py* A program I use to validate the conversion done by `imgconvert.py`
- *mapmeta.py* A program I wrote to figure out how to use the XML Python library, which I ended up using in `imgconvert.py` and `showrawimg.py`
- *nmea.py* A parser for NMEA text sentences captured from GPS
- *maproirotate.py* A program I wrote to test the algorithms for extracting a rotated ROI from a larger image. The algorithm is implemented in `nav.c` function `get_map_patch()` 
- *logger2kml.awk* and AWK script that processed my GPS logger data and converts them to a KML file readable by Google Earth. The resulting KML can be used to display the logged paths on Google Earth.
- *startup.sh* A shell script used to auto start the navigation app in Raspberry Pi. Link through `/etc/rc.local`

## Hardware
### ASCII art block diagram of the hardware

The **GPIO-wiring** files lists the connections between the Raspberry Pi, the GPS Shield, LCD TFT, and pushbuttons

           \|/
            |
            |
    +-------+--------+
    |                |
    | GPS module     |
    |                |
    | ITEAD RoyalTek |
    | REB-4216/      |
    | REB-5216 GPS   |
    |                |
    +-------+--------+
            |
            | Serial UART
            |
    +-------+--------+  +--------------+
    |                |  |              |
    |  Raspberry Pi  |  | LCD 1.8" TFT |
    |                |  | ST7735R      |
    |                |  |              |
    +-------++-------+  +------++------+
            ||                 ||
    ========================================
                        SPI Bus
                        
Not shown
* Push buttons for multi-function control (up/down/lect/right/select)
### Resources
* GPS module [https://www.itead.cc/wiki/Arduino_GPS_shield]
* Interpret NMEA sentences [http://www.gpsinformation.org/dale/nmea.htm] and [http://www.earthpoint.us/Convert.aspx]
* Extracting a rotated ROI, from Dr. Dobd's [http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337]
* Python XML [https://eli.thegreenplace.net/2012/03/15/processing-xml-in-python-with-elementtree]
* Using libxml2 [http://hamburgsteak.sandwich.net/writ/libxml2.txt]
