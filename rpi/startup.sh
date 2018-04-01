#!/bin/bash

GO_FILE='/home/pi/usb/go'

echo "         startup.sh: Starting"

# Check if the USB drive is mounted and if a 'go' files exists
if [ -f $GO_FILE ]; then

    # If 'GO' file exists, then auto-start the navigation app.
    echo "         startup.sh: Found GO file, starting navigation app."
    cd /home/pi/Documents/nav
    sudo ./navigator
    
    # When exiting the navigation app. shutdown the Raspberry Pi
    echo "         startup.sh: Shutting down."
    sudo shutdown now

# If there is no 'GO' file just exit the script
else
    echo "         startup.sh: GO file not found, exiting."
fi

