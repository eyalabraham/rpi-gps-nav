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
#   TODO command line arguments
#        imgconvert.py -i <input_image> -o <output_file> -x <xml_meta-data_file> -t <lat>/<long> -b <lat>/<long>
#
#   Sample XML format for map file meta data:
#
#   <!-- This XML file lists a collection of maps that are
#        available to the GPS device. Each map entry in the list
#        contains the map file's meta data (the map images are raw RGB data!) -->
#   <maps>
#       <!-- One or more map objects describing the area -->
#       <map file="map1.raw" format="RGB565">
#           <!-- Height and width of map in pixels -->
#           <height>800</height>
#           <width>1032</width>
#           <!-- Latitude and longitude of map top-right and bottom-left corners -->
#           <top_right latitude="24.000002" longitude="-71.000000" />
#           <bottom_left latitude="25.000000" longitude="-72.000000" />
#       </map>
#       <map file="map2.raw" format="RGB565">
#           <height>480</height>
#           <width>640</width>
#           <top_right latitude="25.000002" longitude="-73.000000" />
#           <bottom_left latitude="26.000000" longitude="-74.000000" />
#       </map>
#   </maps>
#
#   February 18, 2018
#
#####################################################################

import cv2
import numpy as np
import xml.etree.cElementTree as ET

###############################################################################
#
# main()
#
def main():
    """Load an image file and convert to raw RGB565 format for LCD display."""
    
    #
    # Initializations
    #
    img = cv2.imread('./graphics/town.png')                         # TODO command line parameter, -i <input_image>

    map_img_hight = img.shape[0]
    map_img_width = img.shape[1]
    
    print 'Image shape ', img.shape, '[pixels]'
    
    #
    # Update XML with map's meta data
    #
    maps_meta_data = ET.ElementTree(file='./maps/maps.xml')         # TODO command line parameter, -x <xml_meta-data_file>
    all_maps = maps_meta_data.getroot()
    
    map_of_interest = 'output.raw'                                  # TODO command line parameter, -o <output_file>
    map_to_search = 'map[@file="' + map_of_interest + '"]'          # FIXME this does not work any more, need to change per new XML format
    my_map = None
    for my_map in all_maps.iterfind(map_to_search):
        pass

    if my_map != None:
        # Clear the map element and reset its attributes
        my_map.clear()
        my_map.tag = 'map'
    else:
        # Add a new map element meta-data.
        my_map = ET.SubElement(all_maps, 'map')                     # TODO command line parameter, -o <output_file>
        
    ET.SubElement(my_map, 'file').text = 'output.raw'
    ET.SubElement(my_map, 'height').text = str(map_img_hight)
    ET.SubElement(my_map, 'width').text = str(map_img_width)
    ET.SubElement(my_map, 'top_left', {'latitude':'+00.000001','longitude':'+00.000001'})       # TODO command line parameter, -t <lat>/<long>
    ET.SubElement(my_map, 'bottom_right', {'latitude':'+00.000001','longitude':'+00.000001'})   # TODO command line parameter, -b <lat>/<long>

    maps_meta_data.write('./maps/maps.xml')                         # TODO command line parameter, -x <xml_meta-data_file>

    #
    # Convert the image to RGB565 for LCD
    # and save to raw image file
    #
    img_file = open('./maps/output.raw', 'wb')                      # TODO command line parameter, -o <output_file>

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
