#!/bin/bash

sudo fbcp &

printf "\nCommencing update.\n\n"

cd /home/pi

sudo dpkg --configure -a
sudo apt-get clean
sudo apt-get update --allow-releaseinfo-change
sudo apt-get -y dist-upgrade

cd /home/pi
wget https://github.com/f4dvk/rpi0_sarsat/archive/master.zip -O master.zip
unzip -o master.zip

cp -f -r rpi0_sarsat-master/406 rpi0_sarsat
cp -f -r rpi0_sarsat-master/img rpi0_sarsat
cp -f -r rpi0_sarsat-master/rpi_lcd_1.44 rpi0_sarsat

rm master.zip
rm -rf portsdown-buster-master
cd /home/pi

sudo killall -9 main
echo "Installation rpi0_sarsat"
cd rpi0_sarsat/rpi_lcd_1.44/
make clean
make

cd /home/pi/rpi0_sarsat/406
./install.sh

sudo rm -r /home/pi/leansdr

git clone https://github.com/f4dvk/leansdr.git

cd leansdr/src/apps
make
cp leandvb /home/pi/rpi0_sarsat/rpi_lcd_1.44/bin/

cd /home/pi

printf "\nRebooting\n"

sleep 1

sudo swapoff -a
sudo shutdown -r now

exit