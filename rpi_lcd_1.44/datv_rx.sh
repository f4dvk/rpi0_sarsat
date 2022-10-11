#!/bin/bash

PATHBIN="/home/pi/rpi0_sarsat/rpi_lcd_1.44/bin/"
RX_CONFIG="/home/pi/rpi0_sarsat/rpi_lcd_1.44/config_datv.txt"

get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

sudo fbcp &

MODULATION=$(get_config_var mode $RX_CONFIG)
FEC=$(get_config_var fec $RX_CONFIG)
SYMBOLRATEK=$(get_config_var sr $RX_CONFIG)
let SYMBOLRATE=SYMBOLRATEK*1000

FREQ_OUTPUT=$(get_config_var freq $RX_CONFIG)
FreqHz=$(echo "($FREQ_OUTPUT*1000)/1" | bc )

if [ "$FEC" != "Auto" ]; then
 let FECNUM=FEC
 let FECDEN=FEC+1
 FECDVB="--cr $FECNUM"/"$FECDEN"
else
 FECDVB="--hdlc --packetized"
fi

if [ "$SYMBOLRATEK" -lt 250 ]; then
  SR_RTLSDR=300000
elif [ "$SYMBOLRATEK" -gt 249 ] && [ "$SYMBOLRATEK" -lt 500 ]; then
  SR_RTLSDR=1000000
elif [ "$SYMBOLRATEK" -gt 499 ] && [ "$SYMBOLRATEK" -lt 1000 ]; then
  SR_RTLSDR=1200000
elif [ "$SYMBOLRATEK" -gt 999 ] && [ "$SYMBOLRATEK" -lt 1101 ]; then
  SR_RTLSDR=1250000
else
  SR_RTLSDR=2400000
fi

#if [ "$AUDIO_OUT" == "rpi" ]; then
  # Check for latest Buster update
#  aplay -l | grep -q 'bcm2835 Headphones'
#  if [ $? == 0 ]; then
#    AUDIO_DEVICE="hw:CARD=Headphones,DEV=0"
#  else
#    AUDIO_DEVICE="hw:CARD=ALSA,DEV=0"
#  fi
#else
#  AUDIO_DEVICE="hw:CARD=Device,DEV=0"
#fi

if [ "$MODULATION" == "DVB-S" ]; then
  FASTLOCK="--fastlock"
else
  FASTLOCK=""
fi

KEY="rtl_sdr -g 0 -f $FreqHz -s $SR_RTLSDR - 2>/dev/null "

sudo rm videots >/dev/null 2>/dev/null
sudo killall leandvb >/dev/null 2>/dev/null
#echo " " >/home/pi/tmp/vlc_overlay.txt
sudo killall vlc >/dev/null 2>/dev/null
mkfifo videots

sudo killall fbi >/dev/null 2>/dev/null
sudo fbi -T 1 -noverbose -a /home/pi/rpi0_sarsat/rpi_lcd_1.44/pic/Blank_Black.png
(sleep 0.2; sudo killall -9 fbi >/dev/null 2>/dev/null) &

sudo $KEY\
      | $PATHBIN"leandvb" --fd-info 2 $FECDVB $FASTLOCK --sr $SYMBOLRATE --standard $MODULATION --anf 0 --sampler rrc --rrc-steps 35 --rrc-rej 10 --roll-off 0.35 --ldpc-bf 100 --nhelpers 2 --ts-udp 232.0.0.99:1234 -f $SR_RTLSDR &

#sudo nice -n -30 netcat -u -4 232.0.0.99 1234 < videots &

 #cvlc -I rc --rc-host 127.0.0.1:1111 -f --codec ffmpeg --video-title-timeout=100 \
 #  --width 128 --height 128 \
 #  --sub-filter marq --marq-x 10 --marq-file "/home/pi/tmp/vlc_overlay.txt" \
 #  --gain 3 --alsa-audio-device $AUDIO_DEVICE \
 #  videots >/dev/null 2>/dev/null &

 sudo -u pi cvlc -f --codec ffmpeg --video-title-timeout=100 \
   --width 128 --height 128 \
   udp://@232.0.0.99:1234 >/dev/null 2>/dev/null &
