#!/usr/bin/python
#####################################################################
#
# imgconvert.py
#
#   Accept an image file in any format acceptable by OpenCV, and
#   convert the file into a raw binary image format containing RGB565
#   color format per pixel.
#   Generate and XML description for the image file containing
#   size in pixels and color format
#   TODO command line arguments instead of globals
#        imgconvert.py -i <input_image> -o <output_file> -x <xml_meta-data_file> -t <lat>/<long> -b <lat>/<long>
#
#   Sample XML format for map file meta data:
#
#   <?xml version="1.0" encoding="UTF-8"?>
#   <!-- This XML file lists a collection of maps that are
#        available to the GPS device. Each map entry in the list
#        contains the map file's meta data (the map images are raw RGB data!)
#        The file should be saved as 'maps.xml' for use by the navigation app
#        and stored in the USB drive together with the raw map image files -->
#   <maps>
#       <!-- One or more map objects describing the area -->
#       <map>
#           <file>map1.raw</file>
#           <!-- Height and width of map in pixels -->
#           <height>800</height>
#           <width>1032</width>
#           <!-- Latitude and longitude of map top-right and bottom-left corners -->
#           <top_left latitude="+21.000001" longitude="+22.000001" />
#           <bottom_right latitude="+23.000001" longitude="+24.000001" />
#       </map>
#       <map>
#           <file>map2.raw</file>
#           <height>480</height>
#           <width>640</width>
#           <top_left latitude="+31.000001" longitude="+32.000001" />
#           <bottom_right latitude="+33.000001" longitude="+34.000001" />
#       </map>
#   </maps>#
#
#   February 18, 2018
#
#####################################################################

import cv2
import numpy as np
import xml.etree.cElementTree as ET


#------------- Globals that should be command line options --------------------

INPUT_IMAGE = './graphics/map.png'         # -i <input_image>
OUTOUT_IMAGE = 'map.raw'                   # -o <output_file>
OUTPUT_IMAGE_DIR = './maps/'
XML_MAP_META_DATA = './maps/maps.xml'      # -x <xml_meta-data_file>
TOP_LEFT_LAT = '11.000001'                 # -t <lat>/<long> 
TOP_LEFT_LONG = '-22.000002'
BOTTOM_RIGHT_LAT = '33.000003'             # -b <lat>/<long>
BOTTOM_RIGHT_LONG = '-44.000004'

#------------------------------------------------------------------------------

###############################################################################
#
# main()
#
def main():
    """Load an image file and convert to raw RGB565 format for LCD display."""
    
    #
    # Initializations
    #
    img = cv2.imread(INPUT_IMAGE)

    map_img_hight = img.shape[0]
    map_img_width = img.shape[1]
    
    print 'Image shape ', img.shape, '[pixels]'
    
    #
    # Update XML with new map's meta data
    #
    maps_meta_data = ET.ElementTree(file=XML_MAP_META_DATA)
    all_maps = maps_meta_data.getroot()

    my_map = ET.SubElement(all_maps, 'map')  
    ET.SubElement(my_map, 'file').text = OUTOUT_IMAGE
    ET.SubElement(my_map, 'height').text = str(map_img_hight)
    ET.SubElement(my_map, 'width').text = str(map_img_width)
    ET.SubElement(my_map, 'top_left', {'latitude':TOP_LEFT_LAT,'longitude':TOP_LEFT_LONG})
    ET.SubElement(my_map, 'bottom_right', {'latitude':BOTTOM_RIGHT_LAT,'longitude':BOTTOM_RIGHT_LONG})

    maps_meta_data.write(XML_MAP_META_DATA)

    #
    # Convert the image to RGB565 for LCD
    # and save to raw image file
    #
    img_file = open(OUTPUT_IMAGE_DIR+OUTOUT_IMAGE, 'wb')

    cvt = img.copy()

    for y in range(0,cvt.shape[0]):
        for x in range(0,cvt.shape[1]):
            #
            # Calculate the reduced color bits
            #
            blue = cvt[y,x,0] & 0b11111000
            green = cvt[y,x,1] & 0b11111100
            red = cvt[y,x,2] & 0b11111000
            
            #
            # Save back to converted frame
            #
            cvt[y,x,0] = blue
            cvt[y,x,1] = green
            cvt[y,x,2] = red

            #
            # Build LCD color information byte
            # and store to file
            #
            high_byte = red + (green >> 5)
            low_byte = ((green << 3) & 0b11100000) + (blue >> 3)
            packed_word = bytearray((high_byte,low_byte))
                            
            img_file.write(packed_word)
        
    #
    # Show images
    #        
    cv2.imshow('original', img)
    cv2.imshow('converted', cvt)
    
    print 'Press any key ...'
    cv2.waitKey(0)
    
    #
    # Close file and viewing windows
    #
    img_file.close()
    cv2.destroyAllWindows()

#
# Startup
#
if __name__ == '__main__':
    main()
