#!/bin/bash

echo
echo "------------------------------------------"
echo "-------- Installation rpi0_sarsat --------"
echo "------------------------------------------"

sudo apt-get update
sudo apt-get -y dist-upgrade

sudo apt-get -y install cmake
sudo apt-get -y install sox
sudo apt-get -y install perl
sudo apt-get -y install rtl-sdr

sudo bash -c 'echo -e "\n# for RTL-SDR:\nblacklist dvb_usb_rtl28xxu\n" >> /etc/modprobe.d/blacklist.conf'

cd /home/pi
#wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.60.tar.gz
#tar zxvf bcm2835-1.60.tar.gz
#cd bcm2835-1.60/
#sudo ./configure
#sudo make && sudo make check && sudo make install
#cd /home/pi
#sudo rm -r bcm2835-1.60

wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb
sudo rm wiringpi-latest.deb
cd /home/pi

#git clone https://github.com/juj/fbcp-ili9341.git
#cd fbcp-ili9341
#mkdir build
#cd build
#cmake -DSPI_BUS_CLOCK_DIVISOR=6 -DWAVESHARE_ST7735S_HAT=ON ..
#make -j
#sudo cp fbcp-ili9341 /usr/local/bin/fbcp

echo
echo "------------------------------------------"
echo "------- Téléchargement rpi0_sarsat -------"
echo "------------------------------------------"
wget https://github.com/f4dvk/rpi0_sarsat/archive/master.zip

unzip -o master.zip
mv rpi0_sarsat-main rpi0_sarsat
rm master.zip
cd /home/pi

cd /home/pi/rpi0_sarsat/rpi_lcd_1.44
make clean
make

cd /home/pi/rpi0_sarsat/406
./install.sh

if ! grep -q  "sudo /home/pi/rpi0_sarsat/rpi_lcd_1.44/main &" /home/pi/.bashrc; then
  echo "sudo /home/pi/rpi0_sarsat/rpi_lcd_1.44/main &" >> /home/pi/.bashrc
fi

cd /home/pi

sudo swapoff -a
sudo shutdown -r now

exit
