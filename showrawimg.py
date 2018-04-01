#!/usr/bin/python
#####################################################################
#
# showrawimg.py
#
#   Accept a raw image file in RGB565 that was created with imgconvert.py,
#   and display it to test conversion.
#   TODO command line arguments
#        showrawimg.py -i <input_image> -x <xml_descriptor>
#   TODO read raw image file attributes from an XML segment
#
#   February 18, 2018
#
#####################################################################

import cv2
import numpy as np

###############################################################################
#
# main()
#
def main():
    """Load raw RGB565 image and display in window."""
    
    #
    # Open raw image file
    #
    img_file = open('rpi/res/pattern1.raw', 'rb')
    
    #
    # Get image share in as hight and width in pixels
    # TODO read this from an XML description file
    #      these value *must* match picture dimensions
    #
    img_height = 128
    img_width = 160
    
    shape = (img_height, img_width, 3)
    img = np.zeros(shape, dtype=np.uint8)

    #
    # Read image into image matrix while converting
    # to OpenCV BGR format
    #
    for y in range(0,img.shape[0]):
        for x in range(0,img.shape[1]):
            #
            # Read two bytes at a time
            #
            high_byte = ord(img_file.read(1))
            low_byte = ord(img_file.read(1))
                  
            #
            # Convert to RGB channels
            #
            red = high_byte & 0b11111000
            
            grn1 = (high_byte & 0b00000111) << 5
            grn2 = (low_byte & 0b11100000) >> 3
            green = grn1 + grn2
            
            blue = (low_byte & 0b00011111) << 3
            
            #
            # Store into image matrix
            #
            img[y,x,0] = blue
            img[y,x,1] = green
            img[y,x,2] = red

    #
    # Show image and GPS information
    #        
    cv2.imshow('image', img)
    
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
