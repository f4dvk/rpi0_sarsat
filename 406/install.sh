#! /bin/bash
sudo apt-get -y install gcc build-essential
gcc ./dec406_V6.c -lm -o ./dec406_V6
gcc ./reset_usb.c -lm -o ./reset_usb
sudo chmod a+x scan406.pl
sudo chmod a+x sc*
sudo chmod a+x dec*
sudo chmod a+x reset_usb

exit
