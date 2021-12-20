#!/bin/bash

PATH_406CONFIG="/home/pi/rpi0_sarsat/rpi_lcd_1.44/config.txt"

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

FREQ=$(get_config_var freq $PATH_406CONFIG)
CHECKSUM=$(get_config_var no_checksum $PATH_406CONFIG)

if [ "$CHECKSUM" = "1" ]; then
  CHECKSUM="no_checksum"
else
  CHECKSUM=""
fi

/home/pi/rpi0_sarsat/406/scan406.pl $FREQ"000" 0 $CHECKSUM

exit
