#!/bin/sh
### BEGIN INIT INFO
# Provides: canpi
# Required-Start:    $remote_fs $syslog $network
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start Merg canpi
# Description:       Enable service provided by Merg canpi daemon.
### END INIT INFO

dir="/home/pi/canpi/c/canpi"
cmd="/home/pi/canpi/c/canpi/canpi"
config="${dir}/canpi.cfg"
user=""

name=`basename $0`
pid_file="/var/run/$name.pid"
stdout_log="/var/log/$name.log"
stderr_log="/var/log/$name.err"

source $config
bonjour_file="/etc/avahi/services"
bonjour_template="$dir/services"


get_pid() {
    cat "$pid_file"
}

is_running() {
    [ -f "$pid_file" ] && ps `get_pid` > /dev/null 2>&1
}

setup_bonjour(){
    echo "Configuring the bonjour service"
    #back the old file
    sudo mv $bonjour_file "${bonjour_file}.bak"
    sudo mv $bonjour_template $bonjour_file

    #change the service name
    sudo sed -i 's/SERVICENAME/${service_name}/' $bonjour_file

    #change the port
    sudo sed -i 's/PORT/${service_name}/' $tcpport

    #restart the service
    echo "Restarting the bonjour service"
    sudo /etc/init.d/avahi-daemon restart
    r = `/etc/init.d/avahi-daemon status`
    echo $r|grep "avahi-daemon start/running"
    if [ $? == 0 ]
    then
        echo "Bonjour restarted"
    else
        echo "Failed to restart Bonjour"
    fi
}

setup_ap_mode(){
#set static ip
#start dhcp service
#start hostapd service
}

setup_wifi_mode(){
#set wlan to dhcp
#stop dhcp service
#stop hostapd
}

start_led_watchdog(){

}

stop_led_watchdog(){

}

bootstrap(){
}


case "$1" in
    start)
    if is_running; then
        echo "Already started"
    else
        echo "Starting $name"
        cd "$dir"

        #restart can interface
        sudo /sbin/ip link set can0 down
        sudo /sbin/ip link set can0 up type can bitrate 125000 restart-ms 1000

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
    sudo /sbin/ip link set can0 down
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
        setup_bonjour()
    ;;
    status)
    if is_running; then
        echo "Running"
    else
        echo "Stopped"
        exit 1
    fi
    ;;
    *)
    echo "Usage: $0 {start|stop|restart|status|configure}"
    exit 1
    ;;
esac

exit 0
