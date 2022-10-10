#!/bin/bash

echo
echo "------------------------------------------"
echo "-------- Installation rpi0_sarsat --------"
echo "------------------------------------------"

sudo apt-get update
sudo apt-get -y dist-upgrade

sudo apt-get -y install cmake libusb-1.0-0-dev
sudo apt-get -y install sox
sudo apt-get -y install perl
sudo apt-get -y install git
sudo apt-get -y install fbi vlc ffmpeg
sudo apt-get -y install bc

cd /home/pi
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71/
sudo ./configure
sudo make && sudo make check && sudo make install
cd /home/pi
sudo rm -r bcm2835-1.71

cd /home/pi
wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb
sudo rm wiringpi-latest.deb
cd /home/pi

git clone https://github.com/juj/fbcp-ili9341.git
cd fbcp-ili9341
mkdir build
cd build
cmake -DSPI_BUS_CLOCK_DIVISOR=6 -DWAVESHARE_ST7735S_HAT=ON ..
make -j
sudo cp fbcp-ili9341 /usr/local/bin/fbcp
cd /home/pi

echo
echo "-----------------------------------------------"
echo "----- Installing RTL-SDR Drivers and Apps -----"
echo "-----------------------------------------------"
cd /home/pi
wget https://github.com/f4dvk/rtl-sdr/archive/master.zip
unzip master.zip
mv rtl-sdr-master rtl-sdr
rm master.zip

# Compile and install rtl-sdr
cd rtl-sdr/ && mkdir build && cd build
cmake ../ -DINSTALL_UDEV_RULES=ON
make && sudo make install && sudo ldconfig
sudo bash -c 'echo -e "\n# for RTL-SDR:\nblacklist dvb_usb_rtl28xxu\n" >> /etc/modprobe.d/blacklist.conf'
cd /home/pi

echo
echo "------------------------------------------"
echo "------- Téléchargement rpi0_sarsat -------"
echo "------------------------------------------"

sudo rm -r rpi0_sarsat >/dev/null 2>/dev/null

wget https://github.com/f4dvk/rpi0_sarsat/archive/master.zip

unzip -o master.zip
mv rpi0_sarsat-main rpi0_sarsat
rm master.zip

cd /home/pi/rpi0_sarsat/rpi_lcd_1.44
make clean
make

cd /home/pi/rpi0_sarsat/406
./install.sh

if ! grep -q  "sudo /home/pi/rpi0_sarsat/rpi_lcd_1.44/main &" /home/pi/.bashrc; then
  echo "sudo /home/pi/rpi0_sarsat/rpi_lcd_1.44/main &" >> /home/pi/.bashrc
fi

cd /home/pi

echo
echo "----------------------------------------------"
echo "---------- Installation de LeanDVB -----------"
echo "----------------------------------------------"

git clone https://github.com/f4dvk/leansdr.git

cd leansdr/src/apps
make
cp leandvb /home/pi/rpi0_sarsat/rpi_lcd_1.44/bin/

cd /home/pi

# Auto login
sudo raspi-config nonint do_boot_behaviour B2

# set the framebuffer to 32 bit depth by disabling dtoverlay=vc4-fkms-v3d
echo
echo "----------------------------------------------"
echo "---- Setting Framebuffer to 32 bit depth -----"
echo "----------------------------------------------"

sudo sed -i "/^dtoverlay=vc4-fkms-v3d/c\#dtoverlay=vc4-fkms-v3d" /boot/config.txt

echo
echo "----------------------------------------------"
echo "------------- Setting Hdmi mode --------------"
echo "----------------------------------------------"

echo "hdmi_force_hotplug=1" | sudo tee -a /boot/config.txt
echo "hdmi_group=2" | sudo tee -a /boot/config.txt
echo "hdmi_mode=87" | sudo tee -a /boot/config.txt
echo "hdmi_cvt=128 128 60 1 0 0 0" | sudo tee -a /boot/config.txt
echo "arm_freq=1000" | sudo tee -a /boot/config.txt
echo "core_freq=400" | sudo tee -a /boot/config.txt
echo "force_turbo=1" | sudo tee -a /boot/config.txt

# Réduction temps démarrage sans ethernet
sudo sed -i 's/^TimeoutStartSec.*/TimeoutStartSec=5/' /etc/systemd/system/network-online.target.wants/networking.service
sudo sed -i 's/^#timeout.*/timeout 8;/' /etc/dhcp/dhclient.conf
sudo sed -i 's/^#retry.*/retry 20;/' /etc/dhcp/dhclient.conf

echo "--------------------------------"
echo "----- Complete.  Rebooting -----"
echo "--------------------------------"
sleep 1

sudo swapoff -a
sudo shutdown -r now

exit
