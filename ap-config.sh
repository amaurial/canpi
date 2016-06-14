#!/bin/bash

PATH=/sbin:/bin:/usr/sbin:/usr/bin
. /lib/lsb/init-functions

dir="/home/pi"
canpidir="/home/pi/canpi"

append_to_file(){
   val=$1
   file=$2
   grep -q "$val" "$file" || echo "$val" >> $file
}

config_boot_config(){
   bootconf="/boot/config.txt"
   cp $bootconf "/boot/config.txt.$RANDOM"
   sed -i "s/dtparam=spi=off/dtparam=spi=on/" $bootconf
   #in case the entry is not there
   append_to_file "dtparam=spi=on" $bootconf
   append_to_file "dtoverlay=mcp2515-can0-overlay,oscillator=16000000,interrupt=25" $bootconf
   append_to_file "dtoverlay=spi-bcm2835-overlay" $bootconf
}

add_can_interface(){
  f="/etc/network/interfaces"
  append_to_file "auto can0" $f
  append_to_file "iface can0 can static" $f
  append_to_file "bitrate 125000 restart-ms 1000" $f
}

create_default_canpi_config(){
  conf="$canpidir/canpi.cfg"
  echo "#log config" > $conf
  echo "logfile=\"canpi.log\"" >> $conf
  echo "#INFO,WARN,DEBUG" >> $conf
  echo "loglevel=\"DEBUG\"" >> $conf
  echo "logappend=\"false\"" >> $conf
  echo "#ed service config" >> $conf
  echo "tcpport=5555" >> $conf
  echo "candevice=\"can0\"" >> $conf
  echo "#cangrid config" >> $conf
  echo "can_grid=\"true\"" >> $conf
  echo "cangrid_port=4444" >> $conf
  echo "#Bonjour config" >> $conf
  echo "service_name=\"MYLAYOUT\"" >> $conf
  echo "#network config" >> $conf
  echo "ap_mode=\"False\"" >> $conf
  echo "ap_ssid=\"pi\"" >> $conf
  echo "ap_password=\"12345678\"" >> $conf
  echo "ap_channel=6" >> $conf
  echo "#if not running in ap mode use this" >> $conf
  echo "router_ssid=\"SSID\"" >> $conf
  echo "router_password=\"PASSWORD\"" >> $conf
  echo "node_number=7890" >> $conf
  echo "turnout_file=\"turnout.txt\"" >> $conf
  echo "canid=110" >> $conf
}

echo "Type the Wifi SSID followed by [ENTER]:"
read wssid

echo "Type the Wifi password followed by [ENTER]:"
read wpassword

apt-get update
agt-get install git
apt-get install hostapd
apt-get install isc-dhcp-server

config_boot_config
add_can_interface

#get the code
cd $dir
git clone https://github.com/amaurial/canpi.git

#compile the code
cd canpi
make clean
make all
create_default_canpi_config
#change the router ssid and password
sed -i 's/SSID/$wssid/' "$canpidir/canpi.cfg"
sed -i 's/PASSWORD/$wpassword/' "$canpidir/canpi.cfg"
cd ..
chown -R pi canpi

#backup and move some basic files
mv /etc/hostapd/hostapd.conf /etc/hostapd/hostapd.conf.old
cp "$canpidir/hostapd.conf" /etc/hostapd/

mv /etc/default/hostapd /etc/default/hostapd.old
cp "$canpidir/hostapd" /etc/default/

mv /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.old
cp "$canpidir/dhcpd.conf" /etc/dhcp/

mv /etc/default/isc-dhcp-server /etc/default/isc-dhcp-server.old
cp "$canpidir/isc-dhcp-server" /etc/default 

#copy the configure script
cp "$candir/start_canpi.sh" /etc/init.d/
chmod +x /etc/init.d/start_canpi.sh
update-rc.d start_canpi.sh defaults

#run configure
/etc/init.d/start_canpi.sh configure

