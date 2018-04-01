#####################################################################################
#
#  This make file is for compiling the map based GPS navigation application
#  for Raspberry Pi. Make should be run on a Raspberry Pi.
#
#  Use:
#    clean      - clean environment
#    all        - build all outputs
#
#####################################################################################


#------------------------------------------------------------------------------------
# project directories
#------------------------------------------------------------------------------------
INCDIR = inc
SRCDIR = .
BINDIR = .

#------------------------------------------------------------------------------------
# build tool and options
#------------------------------------------------------------------------------------
CC = gcc
OPT = -Wall -L/usr/local/lib -lbcm2835 -lxml2 -I $(INCDIR) -I/usr/include/libxml2

#------------------------------------------------------------------------------------
# dependencies
#------------------------------------------------------------------------------------
DEPS = test.h pilcd.h util.h config.h vt100lcd.h nav.h
OBJS = main.o test.o pilcd.o util.o vt100lcd.o nav.o

_DEPS = $(patsubst %,$(INCDIR)/%,$(DEPS))

#------------------------------------------------------------------------------------
# build all targets
#------------------------------------------------------------------------------------
%.o: %.c $(_DEPS)
	$(CC) -c -o $@ $< $(OPT)

all: navigator

navigator: $(OBJS)
	$(CC) $(OPT) $^ -o $@

#------------------------------------------------------------------------------------
# sync files and run remote 'make'
# requires ssh key setup to avoid using password authentication
#------------------------------------------------------------------------------------
sync:
	rsync -vrh /home/eyal/Documents/gps/rpi/*  pi@192.168.1.13:/home/pi/Documents/nav
	rsh pi@192.168.1.13 "cd /home/pi/Documents/nav && make"

#------------------------------------------------------------------------------------
# cleanup
#------------------------------------------------------------------------------------
.PHONY: clean

clean:
	rm -f navigator
	rm -f *.o
	rm -f *.bak

