#!/bin/bash

sudo dpkg -i /home/bacs/host/arduino/libftdi1_0.20-4build1_amd64.deb

sudo dpkg -i /home/bacs/host/arduino/avrdude_6.2-5_amd64.deb

sudo killall BACSDuoUart DuoUart.sh

avrdude -C /etc/avrdude.conf -v -patmega328p -Uflash:w:"./BACSDuo.ino.hex":i -carduino -b 57600 -P "/dev/ttyUSBJJ"

#cd /home/bacs/host
#./DuoUart.sh &
