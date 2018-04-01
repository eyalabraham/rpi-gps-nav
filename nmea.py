###########################################################
# nmea.py
#
#   test processing NMEA data format read from GPS module
#   format:
#       $GP<sig>,<param0>, ... ,<paramN>*<check-sum>
#       <sig> GGA Global positioning system fixed data
#             GSA GNSS DOP and active satellites
#             GSV GNSS satellites in view
#             RMC Recommended minimum specific GNSS data
#             GLL Geographic position latitude/longitude
#             VTG Course over ground and ground speed
#
###########################################################

def main():
    with open('test-capture') as f:
        line = f.readline()
        while line:
            process_nmea(line.strip())
            line = f.readline()

def process_nmea(line):
    line = line.lstrip('$')
    nmea_parts = line.partition('*')
    checksum = int(nmea_parts[2], 16)
    nmea_param = nmea_parts[0].split(',')

    checksum_test = 0
    for c in nmea_parts[0]:
        checksum_test ^= ord(c)

    if checksum == checksum_test:
        checksum_status = '[ok ]'
    else:
        checksum_status = '[err]'

    #
    # Interpret NMEA sentences
    # source: http://www.gpsinformation.org/dale/nmea.htm
    #         http://www.earthpoint.us/Convert.aspx
    #
    if nmea_param[0] == 'GPGGA':
        print checksum_status, nmea_parts[0]
        if int(nmea_param[6]) == 1:
            print '      Fix time {}:{}:{}'.format(nmea_param[1][:2][:2], nmea_param[1][2:4], nmea_param[1][4:6])
            print '      Satelites {}'.format(nmea_param[7])
            latitude = float(nmea_param[2][:2]) + float(nmea_param[2][2:]) / 60
            if nmea_param[3] == 'S':
                latitude *= -1
            print '      Lat {:<.6f}'.format(latitude)
            longitude = float(nmea_param[4][:3]) + float(nmea_param[4][3:]) / 60
            if nmea_param[5] == 'W':
                longitude *= -1
            print '      Long {:<.6f}'.format(longitude)
        else:
            print '      Invalid fix.'

    elif nmea_param[0] == 'GPGSA':
        pass

    elif nmea_param[0] == 'GPGSV':
        pass

    elif nmea_param[0] == 'GPRMC':
        print checksum_status, nmea_parts[0]
        if nmea_param[2] == 'A':
            ground_speed = float(nmea_param[7]) * 1.150779
            print '      Ground speed {:<.2f} [mph]'.format(ground_speed)
            heading = float(nmea_param[8])
            print '      Heading {:<.2f} [deg]'.format(heading)
        else:
            print '      Invalid fix.'

    elif nmea_param[0] == 'GPGLL':
        pass

    elif nmea_param[0] == 'GPVTG':
        pass

if __name__ == '__main__':
    main()