############################################################
#
# logger2kml.awk
#
#   This script reads in GPS logger.csv file and outputs
#   a KML formatted file for displaying the route represented by
#   the way points in Google Earth
#
#   Logger file format:
#
#   #
#   # GPS logger
#   #
#   #logged_points,gga_time,latitude,longitude,ground_spd,heading
#   < ... CSV data points, one line per way point ... >
#
#   March 27, 2018
#
############################################################

        #
        # Print out KML definitions and standard tags
        # for a line tracking ground altitude
        #
        # Line color: The order of expression is aabbggrr,
        #             where aa=alpha (00 to ff); bb=blue (00 to ff); gg=green (00 to ff); rr=red (00 to ff).
        # Color tool: http://www.zonums.com/gmaps/kml_color/
        #
BEGIN   { colors[1]  = "7f00ffff"
          colors[2]  = "7f1400FF"
          colors[3]  = "7fFF7800"
          colors[4]  = "7f7800F0"
          colors[5]  = "7f1478FF"
          colors[6]  = "7f14F000"
          colors[7]  = "7fFFFFFF"
          colors[8]  = "ff000000"
          colors[9]  = "7fF00014"
          colors[10] = "7fF0FF14"
          
          MAX_COLORS = 10
          current_color = 1
          first_path = 1
          
          print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
          print "<kml xmlns=\"http://www.opengis.net/kml/2.2\">"
          print "    <Document>"
          print "       <name>Paths</name>"
          print "       <description>GPS logger path</description>"
        }
        
        #
        # If a path list starts, close the previous 'Placemark' tag
        # switch colors and start a new path
        #
/logger/{ if ( first_path == 1 )
          {
            first_path = 0
          }
          else
          {
            print "               </coordinates>"
            print "           </LineString>"
            print "       </Placemark>"
            current_color++
            if ( current_color > MAX_COLORS )
                current_color = 1
          }
          
          #
          # Define a path style
          #
          print "       <Placemark>"
          print "           <name>Absolute Extruded</name>"
          print "           <description>Path line</description>"
          print "           <Style>"
          print "               <LineStyle>"
          printf ("                   <color>%8s</color>\n", colors[current_color])
          print "                   <width>4</width>"
          print "               </LineStyle>"
          print "           </Style>"
          print "           <LineString>"
          print "               <extrude>0</extrude>"
          print "               <tessellate>1</tessellate>"
          print "               <altitudeMode>clampToGround</altitudeMode>"
          print "               <coordinates>"
        }
        
        #
        # Generate the long/lat data from the logger file
        #
/^[^#]/ { split($0, fields, ",")
          printf("                %s,%s\n", fields[4], fields[3])
        }
        
        #
        # Print out the KML file closing tags
        #
END     { print "               </coordinates>"
          print "           </LineString>"
          print "       </Placemark>"
          print "    </Document>"
          print "</kml>"
        }
