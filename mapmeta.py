######################################################################
#
# mapmeta.py
#
#   Test program to parse map meta data XML file
#
#   Source: https://eli.thegreenplace.net/2012/03/15/processing-xml-in-python-with-elementtree
#           https://docs.python.org/2/library/xml.etree.elementtree.html
#
#   Created Feb.21.2018
#
######################################################################

import xml.etree.cElementTree as ET

maps_meta_data = ET.ElementTree(file='maps/sample.xml')

all_maps = maps_meta_data.getroot()

map_of_interest = 'map2.raw'                            # TODO should be input as command line argument

map_to_search = 'map[@file="' + map_of_interest + '"]'  # FIXME this won't work becaue I changed the XML struture

#
# Find the map and display its meta-data.
#
my_map = None
for my_map in all_maps.iterfind(map_to_search):
    pass

if my_map != None:

    print 'map file name: ', my_map.attrib['file']
    print '\tcolor encoding: ', my_map.attrib['format']

    #
    # Find tags of the specific map and display them.
    #

    # Map's pixel height
    meta_data_element = None
    for meta_data_element in my_map.iterfind('height'):
        pass
    if meta_data_element != None:
        print '\timage height: ',meta_data_element.text
    else:
        print 'Missing map data element!'

    # Map's pixel width
    meta_data_element = None
    for meta_data_element in my_map.iterfind('width'):
        pass
    if meta_data_element != None:
        print '\timage width: ',meta_data_element.text
    else:
        print 'Missing map data element!'

    # Map's top right GPS coordinate
    meta_data_element = None
    for meta_data_element in my_map.iterfind('top_right'):
        pass
    if meta_data_element != None:
        print '\ttop_right: ', meta_data_element.attrib
    else:
        print 'Missing map data element!'

else:
    print 'Map meta data not found!'

#
# Add a new map element meta-data.
#
new_map = ET.SubElement(all_maps, 'map', {'file':'map4.raw', 'format':'RGB565'})
ET.SubElement(new_map,'height').text='1024'
ET.SubElement(new_map,'width').text='2048'
ET.SubElement(new_map,'top_right', {'latitude':'99.999999', 'longitude':'99.999999'})
ET.SubElement(new_map,'bottom_left', {'latitude':'99.999999', 'longitude':'99.999999'})

#
# Save the modified XML file.
#
maps_meta_data.write('sample_new.xml')
