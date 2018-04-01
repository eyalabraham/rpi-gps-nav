######################################################################
#
# maproirotate.py
#
#   Test program to test map segment rotation algorithm
#
# These two look the same:
# 1) Source: http://eab.abime.net/showthread.php?t=29492
# 2) From Dr. Dobd's: http://www.drdobbs.com/architecture-and-design/fast-bitmap-rotation-and-scaling/184416337
#
# Some other resources:
# - Something that seems more reliable: https://github.com/mauryquijada/image-manipulation-in-c/blob/master/rotate.c
# - Other resources: http://www.leptonica.com/local-sources.html
#
# Assuming width and height are integers with the image's dimensions
#
# (1) Definitely pull this:
#
# 		double sinma = sin(-angle);
# 		double cosma = cos(-angle);
#
#     and this:
#
# 		int hwidth = width / 2;
# 		int hheight = height / 2;
#
#     out of the inner loop, that way you only have to compute the sines and cosines once per rotation,
#     not once per pixel which is totally pointless.
#
# (2) If you're only using integer angles and can spare the memory, you might want to compute
#     the 360 values for cos and sin in advance, throw them in some array and then just read
#     them from there, that should be a lot faster than computing them for every pixel
#
# for(int x = 0; x < width; x++) {
#   for(int y = 0; y < height; y++) {
#       int hwidth = width / 2;
#       int hheight = height / 2;
#
#       int xt = x - hwidth;
#       int yt = y - hheight;
#
#       double sinma = sin(-angle);
#       double cosma = cos(-angle);
#
#       int xs = (int)round((cosma * xt - sinma * yt) + hwidth);
#       int ys = (int)round((sinma * xt + cosma * yt) + hheight);
#
#       if(xs >= 0 && xs < width && ys >= 0 && ys < height) {
# 			/* set target pixel (x,y) to color at (xs,ys) */
# 		} else {
# 			/* set target pixel (x,y) to some default background */
# 		}
# 	}
# }
#
#   Created March.28.2018
#
######################################################################

import cv2
import numpy as np
import math

###############################################################################
#
# main()
#
def main():
    """Load map image and extract a rotated ROI."""

    #
    # Initializations
    #
    #cv2.namedWindow('map', cv2.WINDOW_GUI_NORMAL+cv2.WINDOW_AUTOSIZE)
    cv2.namedWindow('roi', cv2.WINDOW_GUI_NORMAL+cv2.WINDOW_AUTOSIZE)
    font = cv2.FONT_HERSHEY_SIMPLEX

    img = cv2.imread('graphics/town.png')
    img_height = img.shape[0]
    img_width = img.shape[1]
    print 'IMG: shape ', img.shape

    #
    # Create a destination frame for the rotated ROI output image
    #
    roi_center = (294, 297)
    roi_img_height = 128
    roi_img_width = 160
    theta = 3.1415926

    shape = (roi_img_height, roi_img_width, 3)
    roi_img = np.zeros(shape, dtype=np.uint8)
    print 'ROI: shape ', roi_img.shape

    #
    # Extract rotated ROI
    #
    hwidth = roi_img_width / 2
    hheight = roi_img_height / 2

    print 'Press any key to rotate, <q> or <ESC> to exit'
    
    for angl in range(0,360,3):
    
        theta = angl * math.pi / 180.0
        
        sin_theta = math.sin(-theta)
        cos_theta = math.cos(-theta)

        for y in range(0,roi_img_height):
            for x in range(0,roi_img_width):

                xt = x - hwidth
                yt = y - hheight

                u = int(xt * cos_theta - yt * sin_theta) + roi_center[0]
                v = int(xt * sin_theta + yt * cos_theta) + roi_center[1]

                #u = int(cos_mtheta * xt - sin_mtheta * yt)
                #v = int(sin_mtheta * xt + cos_mtheta * yt)

                if (u >= 0 and u < img_width and v >= 0 and v < img_height):
                    roi_img[y,x] = img[v,u]
                else:
                    roi_img[y,x] = (0,0,0)

        #
        # Draw ROI markers on original image
        #
        #cv2.rectangle(img, (map_x-80,map_y-64), (map_x+80,map_y+64), (255,0,0), 2)

        #
        # Show images
        #
        cv2.imshow('map', img)
        cv2.imshow('roi', roi_img)

        #
        # Wait for 'q' key to exit
        #
        key = cv2.waitKey(0) & 0xFF
        if key == ord('q') or key == 27:
            break

    cv2.destroyAllWindows()

#
# Startup
#
if __name__ == '__main__':
    main()
