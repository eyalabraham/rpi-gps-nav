#####################################################################
#
# mapcoord.py
#
#   Assess ideas for mapping GPS coordinate onto a bitmap map that
#   was screen-captured from Google maps.
#   Map GPS coordinates to pixel coordinates and assess linearity 
#   and accuracy, requires transforms etc.
#
#   TODO:The program also tests algorithm for latitude and longitude
#   conversion to decimal coordinates to/from degrees minutes seconds (DMS)
#   with latlong_dec2dms() and latlog_dms2dec().
#
#   Image GPS coordinate
#    Top left:     40.7528705, -73.9967314 (10th and W 34th, NYC)
#    Bottom right: 40.7399611, -73.9766969 (1st and E28th, NYC)
#
#   February 17, 2018
#
#####################################################################

import cv2
import numpy as np

###############################################################################
#
# main()
#
def main():
    """Load map image and plot GPS coordinates on the image."""
    
    global map_x, map_y
    
    #
    # Initializations
    #
    map_x, map_y = 0, 0
    
    map_top_left = (40.7528705, -73.9967314)
    map_bottom_right = (40.7399611, -73.9766969)
    
    #cv2.namedWindow('town', cv2.WINDOW_GUI_NORMAL+cv2.WINDOW_AUTOSIZE)
    cv2.namedWindow('town')
    cv2.setMouseCallback('town', mouse_pos)
    font = cv2.FONT_HERSHEY_SIMPLEX
    
    img = cv2.imread('graphics/town.png')
    temp_img = img.copy()
    
    map_img_hight = img.shape[0]
    map_img_width = img.shape[1]
    
    ns_per_pix = 1000000.0 * abs(map_top_left[0] - map_bottom_right[0]) / map_img_hight
    ew_per_pix = 1000000.0 * abs(map_top_left[1] - map_bottom_right[1]) / map_img_width
    
    print 'map: shape ', img.shape
    
    #
    # Track mouse position over map image and display calculated GPS
    # coordinates under the mouse pointer
    #
    while (True):
        #
        # Draw markers around mouse position on map
        # Crosshair and rectangle representing LCD view
        #
        cv2.line(temp_img, (map_x,0), (map_x,map_img_hight), (0,0,255), 1)
        cv2.line(temp_img, (0,map_y), (map_img_width,map_y), (0,0,255), 1)
        cv2.rectangle(temp_img, (map_x-80,map_y-64), (map_x+80,map_y+64), (255,0,0), 2)

        #
        # calculate GPS coordinates and print on image
        #
        ns_coord = map_top_left[0] - (map_y*ns_per_pix)/1000000.0
        ew_coord = map_top_left[1] + (map_x*ew_per_pix)/1000000.0
        coord_text = '{:.6f} {:.6f}'.format(ns_coord, ew_coord)
        
        cv2.putText(temp_img, coord_text, (1, map_img_hight-5), font, 0.4, (255, 0, 0), 1, cv2.LINE_AA)
        
        #
        # Show image and GPS information
        #
        cv2.imshow('town', temp_img)
        temp_img = img.copy()
    
        key = cv2.waitKey(33) & 0xFF
        if key == ord('q') or key == 27:
            break
            
    cv2.destroyAllWindows()

###########################################################
#
# mouse_pos()
#
#   Mouse event callback function.
#   Used to show ROI and map point of interest
#
#   param:  event type, mouse coordinates and event parameters
#   return: nothing, marks a frame region for tracker initialization
#
def mouse_pos(event, x, y, flags, param):
    """Mouse callback that shows region of LCD display and GPS point."""

    global map_x, map_y

    if event == cv2.EVENT_MOUSEMOVE:
        map_x, map_y = x, y


#
# Startup
#
if __name__ == '__main__':
    main()
