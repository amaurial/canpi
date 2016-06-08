#!/bin/bash

apt-get update
apt-get install hostapd
apt-get install isc-dhcp-server
mv /etc/hostapd/hostapd.conf /etc/hostapd/hostapd.conf.old
cp hostapd.conf /etc/hostapd/

mv /etc/default/hostapd /etc/default/hostapd.old
cp hostapd /etc/default/

mv /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.old
cp dhcpd.conf /etc/dhcp/

mv /etc/default/isc-dhcp-server /etc/default/isc-dhcp-server.old
cp isc-dhcp-server /etc/default 

