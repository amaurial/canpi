#!/bin/bash
### BEGIN INIT INFO
# Provides: canpi
# Required-Start:    $remote_fs $syslog $network
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start Merg canpi
# Description:       Enable service provided by Merg canpi daemon.
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
. /lib/lsb/init-functions

dir="/home/pi/canpi"
cmd="/home/pi/canpi/canpi"
webdir="$dir/webserver"
webcmd="/usr/bin/python $webdir/canpiconfig.py 80"
config="${dir}/canpi.cfg"
user=""

name=`basename $0`
pid_file="/var/run/$name.pid"
stdout_log="/var/log/$name.log"
stderr_log="/var/log/$name.err"

webname="canpiconfig"
web_pid_file="/var/run/$webname.pid"
web_stdout_log="/var/log/$webname.log"
web_stderr_log="/var/log/$webname.err"

#clean spaces and ';'
sed -i 's/ = /=/g' $config
sed -i 's/;//g' $config
source $config

#Bonjour files
bonjour_file="/etc/avahi/services/multiple.service"
bonjour_template="$dir/multiple.service"

#wpa supplicant files
wpa_file="/etc/wpa_supplicant/wpa_supplicant.conf"
wpa_template="$dir/wpa_supplicant.conf"

#hostapd files
hostapd_file="/etc/hostapd/hostapd.conf"
hostapd_template="$dir/hostapd.conf"
default_hostapd_file="/etc/default/hostapd"
safe_hostapd_file="$dir/hostapd"

#dhcp files
default_dhcp_file="/etc/default/isc-dhcp-server"
safe_dhcp_file="$dir/isc-dhcp-server"

#interface files
iface_file="/etc/network/interfaces"
iface_file_wifi="$dir/interfaces.wifi"
iface_file_ap="$dir/interfaces.ap"


get_pid() {
    cat "$pid_file"
}

is_running() {
    [ -f "$pid_file" ] && ps `get_pid` > /dev/null 2>&1
}

get_web_pid() {
    cat "$web_pid_file"
}

is_web_running() {
    [ -f "$web_pid_file" ] && ps `get_web_pid` > /dev/null 2>&1
}

is_avahi_running(){
    r=`/etc/init.d/avahi-daemon status`
    echo $r | grep "active (running)"
}

is_dhcp_running(){
    r=`/etc/init.d/isc_dhcp-server status`
    echo $r | grep "active (running)"
}

is_hostapd_running(){
    r=`/etc/init.d/hostapd status`
    echo $r | grep "active (running)"
}

setup_bonjour() {
    echo "Configuring the bonjour service"
    #back the old file
    #mv $bonjour_file "${bonjour_file}.bak"
    cp $bonjour_template "${bonjour_template}.tmp"

    #change the service name
    sed -i 's/SERVICENAME/'"$service_name"'/' "${bonjour_template}.tmp"

    #change the port
    sed -i 's/PORT/'"$tcpport"'/' "${bonjour_template}.tmp"
    mv "${bonjour_template}.tmp" $bonjour_file

    #restart the service
    #echo "Restarting the bonjour service"
    #/etc/init.d/avahi-daemon restart
    #sleep 1
    #r=`/etc/init.d/avahi-daemon status`
    #echo $r | grep "active (running)"
    if is_avahi_running;
    then
        echo "Bonjour restarted"
    else
        echo "Failed to restart Bonjour"
    fi
}

setup_wpa_supplicant(){
    echo "Configuring the WPA supplicant"
    #back the old file
    sudo mv $wpa_file "${wpa_file}.bak"
    sudo cp $wpa_template $wpa_file

    #change the ssid
    sudo sed -i 's/SSID/'"$router_ssid"'/' $wpa_file

    #change the password
    sudo sed -i 's/PASSWORD/'"$router_password"'/' $wpa_file

}

setup_hostapd(){
    echo "Configuring the hostapd"
    #back the old file
    sudo mv $hostapd_file "${hostapd_file}.bak"
    sudo cp $hostapd_template $hostapd_file

    #change the ssid
    sudo sed -i 's/SSID/'"$ap_ssid"'/' $hostapd_file

    #change the password
    sudo sed -i 's/PASSWORD/'"$ap_password"'/' $hostapd_file

    #change the channel
    sudo sed -i 's/CHANNEL/'"$ap_channel"'/' $hostapd_file
}

setup_hostapd_no_password(){
    echo "Configuring the hostapd with no password"
    #back the old file
    sudo mv $hostapd_file "${hostapd_file}.bak"
    sudo cp "${hostapd_template}.nopassword" $hostapd_file

    #change the ssid
    sudo sed -i 's/SSID/'"$ap_ssid"'/' $hostapd_file

    #change the channel
    sudo sed -i 's/CHANNEL/'"$ap_channel"'/' $hostapd_file
}



setup_iface_wifi(){
    #back the old file
    sudo mv $iface_file "${iface_file}.bak"
    sudo cp $iface_file_wifi $iface_file
}

setup_iface_ap(){
    #back the old file
    sudo mv $iface_file "${iface_file}.bak"
    sudo cp $iface_file_ap $iface_file
}

remove_hostapd_default(){
    #back the old file
    sudo mv $default_hostapd_file "${default_hostapd_file}.bak"
}

remove_dhcp_default(){
    #back the old file
    sudo mv $default_dhcp_file "${default_dhcp_file}.bak"
}

restore_hostapd_default(){
    #back the old file
    sudo cp $safe_hostapd_file $default_hostapd_file
}

restore_dhcp_default(){
    #back the old file
    sudo cp $safe_dhcp_file $default_dhcp_file
}

setup_ap_mode()
{
    echo "Configuring to AP mode"


    sleep 1
    sudo /etc/init.d/hostapd stop
    sleep 1
    sudo /etc/init.d/isc-dhcp-server stop


    echo "Restarting wlan0"
    sudo ip link set wlan0 down
    setup_iface_ap
    if [[ $ap_no_password == "true" || $ap_no_password == "True" || $ap_no_password == "TRUE" ]];then
        setup_hostapd_no_password
    else
        setup_hostapd
    fi

    sleep 1
    sudo ip link set wlan0 up

    sleep 2
    sudo /etc/init.d/hostapd start
    sleep 1
    sudo /etc/init.d/isc-dhcp-server start

    echo "Configuring the hostapd daemon"
    sudo /usr/sbin/update-rc.d hostapd defaults
    echo "Configuring DHCP server"
    sudo /usr/sbin/update-rc.d isc-dhcp-server defaults
}

setup_wifi_mode(){
    echo "Configuring to wifi mode"
    setup_iface_wifi
    setup_wpa_supplicant

    echo "Stoping wlan0"
    sudo ip link set wlan0 down

    echo "Stoping the hostapd service"
    sudo /etc/init.d/hostapd stop
    sleep 1

    echo "Stoping the DHCP service"
    sudo /etc/init.d/isc-dhcp-server stop
    sleep 1

    #echo "Starting wlan0"
    #sudo ip link set wlan0 up
    #sudo /etc/init.d/networking rstart
    #sleep 1
    #remove_hostapd_default
    #remove_dhcp_default
    sudo /usr/sbin/update-rc.d hostapd remove
    sleep 1
    sudo /usr/sbin/update-rc.d isc-dhcp-server remove
}

start_led_watchdog(){
   echo "watchdog"
}

stop_led_watchdog(){
   echo "watchdog"
}

bootstrap(){
   echo "bootstrap"
}

start_webserver(){
    if is_web_running; then
        echo "Web service already started"
    else
        echo "Starting $webname"
        cd "$webdir"

        if [ -z "$user" ]; then
            sudo $webcmd >> "$web_stdout_log" 2>> "$web_stderr_log" &
        else
            sudo -u "$user" $webcmd >> "$web_stdout_log" 2>> "$web_stderr_log" &
        fi
        echo $! > "$web_pid_file"
        if ! is_web_running; then
            echo "Unable to start, see $web_stdout_log and $web_stderr_log"
            exit 1
        fi
    fi
}

stop_webserver(){
    if is_web_running; then
        echo -n "Stopping $webname.."
        kill `get_web_pid`
        for i in {1..10}
        do
            if ! is_web_running; then
                break
            fi

            echo -n "."
            sleep 1
        done
        echo

        if is_web_running; then
            echo "$webname not stopped; may still be shutting down or shutdown may have failed"
            #exit 1
        else
            echo "Stopped"
            if [ -f "$web_pid_file" ]; then
                rm "$web_pid_file"
            fi
        fi
    else
        echo "$webname not running"
    fi
}

case "$1" in
    start)

    setup_bonjour
    start_webserver

    if is_running; then
        echo "Already started"
    else
        echo "Starting $name"
        cd "$dir"

        #restart can interface
        #sudo /sbin/ip link set can0 down
        #sudo /sbin/ip link set can0 up type can bitrate 125000 restart-ms 1000

        if [ -z "$user" ]; then
            sudo $cmd >> "$stdout_log" 2>> "$stderr_log" &
        else
            sudo -u "$user" $cmd >> "$stdout_log" 2>> "$stderr_log" &
        fi
        echo $! > "$pid_file"
        if ! is_running; then
            echo "Unable to start, see $stdout_log and $stderr_log"
            exit 1
        fi
    fi
    ;;
    stop)

    stop_webserver
    #sudo /sbin/ip link set can0 down
    if is_running; then
        echo -n "Stopping $name.."
        kill `get_pid`
        for i in {1..10}
        do
            if ! is_running; then
                break
            fi

            echo -n "."
            sleep 1
        done
        echo

        if is_running; then
            echo "Not stopped; may still be shutting down or shutdown may have failed"
            exit 1
        else
            echo "Stopped"
            if [ -f "$pid_file" ]; then
                rm "$pid_file"
            fi
        fi
    else
        echo "Not running"
    fi
    ;;
    startcanpi)

    setup_bonjour
    if is_running; then
        echo "Already started"
    else
        echo "Starting $name"
        cd "$dir"

        #restart can interface
        #sudo /sbin/ip link set can0 down
        #sudo /sbin/ip link set can0 up type can bitrate 125000 restart-ms 1000

        if [ -z "$user" ]; then
            sudo $cmd >> "$stdout_log" 2>> "$stderr_log" &
        else
            sudo -u "$user" $cmd >> "$stdout_log" 2>> "$stderr_log" &
        fi
        echo $! > "$pid_file"
        if ! is_running; then
            echo "Unable to start, see $stdout_log and $stderr_log"
            exit 1
        fi
    fi
    ;;
    stopcanpi)
    if is_running; then
        echo -n "Stopping $name.."
        kill `get_pid`
        for i in {1..10}
        do
            if ! is_running; then
                break
            fi

            echo -n "."
            sleep 1
        done
        echo

        if is_running; then
            echo "Not stopped; may still be shutting down or shutdown may have failed"
            exit 1
        else
            echo "Stopped"
            if [ -f "$pid_file" ]; then
                rm "$pid_file"
            fi
        fi
    else
        echo "Not running"
    fi
    ;;
    restartcanpi)
    $0 stopcanpi
    if is_running; then
        echo "Unable to stop, will not attempt to start"
        exit 1
    fi
    $0 startcanpi
    ;;
    restart)
    $0 stop
    if is_running; then
        echo "Unable to stop, will not attempt to start"
        exit 1
    fi
    $0 start
    ;;
    configure)
        echo "Configuring the services"
        if [[ $ap_mode == "true" || $ap_mode == "True" || $ap_mode == "TRUE" ]];then
           echo "Starting configuration for AP Mode"
           setup_ap_mode
        else
           echo "Starting configuration for Wifi Mode"
           setup_wifi_mode
        fi
        setup_bonjour
        sleep 1
        echo "Rebooting"
        sudo reboot
    ;;
    status)
    if is_web_running; then
        echo "Config is Running"
    else
        echo "Config Stopped"
    fi

    if is_running; then
        echo "Canpi is Running"
    else
        echo "Canpi Stopped"
        exit 1
    fi
    ;;
    *)
    echo "Usage: $0 {start|stop|restart|status|configure|startcanpi|stopcanpi|restartcanpi}"
    exit 1
    ;;
esac

exit 0
