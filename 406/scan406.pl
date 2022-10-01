#!/usr/bin/env perl
# Script perl pour scanner la fréquence des balises 406MHz

# Script perl inspiré  de  "rtlsdr_scan" des auteurs de "RS-master"
# https://github.com/rs1729

# ce script utilise "rtl_power" et "rtl_fm" pour la réception sur une clé SDR
# https://github.com/keenerd/rtl-sdr

# ainsi que "reset_dvbt"
# http://askubuntu.com/questions/645/how-do-you-reset-a-usb-device-from-the-command-line

# le décodage se fait avec "dec406_v6" contenu dans "scan406.zip"
# http://jgsenlis.free.fr/406_PI.html
# Auteur F4EHY 8-6-2020

use POSIX qw(strftime);
use strict;
use warnings;
my $ppm = 0;
my $line;
my $utc;
my @db;
my $i;
my $j;
my $freq= "406.025M";
my $dec = '/home/pi/rpi0_sarsat/406/dec406_V6 --100 --M3';
my $dec1 = '/home/pi/rpi0_sarsat/406/dec406_V6 --100 --M3 --no_checksum';
my $no_checksum = 0;
my $filter = "lowpass 3000 highpass 400"; #highpass de 10Hz à 400Hz selon la qualité du signal

my $largeur = "12k";

my $var=@ARGV;
if ($var<1)
	{print "\n SYNTAXE:  scan406.pl  freq [decalage_ppm] [no_checksum]\n";
	print " Exemple:\n	scan406.pl 406M\n	scan406.pl 406M no_checksum\n\n";
	exit(0);
	}

for (my $i=0;$i<@ARGV;$i++)
{
	#print "\n $ARGV[$i]";
	if ($i==0)
	{$freq=$ARGV[0];
	}
	if ($i==1)
	{$ppm=$ARGV[1];
	}
	if ($i==2)
	{  if ($ARGV[2] eq 'no_checksum')
	   {
	     $no_checksum=1;
	   }
	}
}


while (1) {
    reset_dvbt();
    if ($no_checksum == 1)
    {
      $dec=$dec1;
    }
    printf "\nLancement ...";
    #$utc = strftime(' %d %m %Y   %Hh%Mm%Ss', gmtime);
    #printf "$utc UTC";
    system("rtl_fm -p $ppm -M fm -s $largeur -R 1 -f $freq 2>/dev/null |\
	    sox -t raw -r $largeur -e s -b 16 -c 1 - -t wav - $filter 2>/dev/null |\
	    $dec ");
}

## http://askubuntu.com/questions/645/how-do-you-reset-a-usb-device-from-the-command-line
## usbreset.c -> reset_usb

sub reset_dvbt {
    my @devices = split("\n",`lsusb`);
    foreach my $line (@devices) {
        if ($line =~ /\w+\s(\d+)\s\w+\s(\d+):\sID\s([0-9a-f]+):([0-9a-f]+).+Realtek Semiconductor Corp\./) {
            if ($4 eq "2832"  ||  $4 eq "2838") {
                system("/home/pi/rpi0_sarsat/406/reset_usb /dev/bus/usb/$1/$2");
            }
        }
    }
}
