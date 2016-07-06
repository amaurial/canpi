#ifndef __UTILS_H
#define __UTILS_H

#include <stdexcept>
#include <sstream>
#include <string>
#include "libconfig.h++"

//################# Config files definitions
#define TAG_CANID        "canid"
#define TAG_GRID_PORT    "cangrid_port"
#define TAG_TCP_PORT     "tcpport"
#define TAG_NN           "node_number"
#define TAG_AP_MODE      "ap_mode"
#define TAG_CAN_GRID     "can_grid"
#define TAG_SSID         "ap_ssid"
#define TAG_PASSWD       "ap_password"
#define TAG_ROUTER_SSID  "router_ssid"
#define TAG_ROUTER_PASSWD  "router_password"
#define TAG_LOGLEVEL     "loglevel"
#define TAG_LOGFILE      "logfile"
#define TAG_SERV_NAME    "service_name"
#define TAG_LOGAPPEND    "logappend"
#define TAG_TURNOUT      "turnout_file"
#define TAG_CANDEVICE    "candevice"
#define TAG_APCHANNEL    "ap_channel"
#define TAG_BP           "button_pin"
#define TAG_GL           "green_led_pin"
#define TAG_YL           "yellow_led_pin"
#define TAG_FNMOM        "fn_momentary"
#define TAG_WARN         "WARN"
#define TAG_INFO         "INFO"
#define TAG_DEBUG        "DEBUG"
#define TAG_NO_PASSWD    "ap_no_password"
#define TAG_CREATE_LOGFILE "create_log_file"


using namespace libconfig;
using namespace std;

//byte definition
typedef unsigned char byte;
enum ClientType {ED,GRID};
enum TurnoutState {CLOSED,THROWN,UNKNOWN};

#define INTERROR 323232

//custom exception class
class my_exception : public std::runtime_error {
    std::string msg;
public:
    my_exception(const std::string &arg, const char *file, int line) :
    std::runtime_error(arg) {
        std::ostringstream o;
        o << file << ":" << line << ": " << arg;
        msg = o.str();
    }
    ~my_exception() throw() {}
    const char *what() const throw() {
        return msg.c_str();
    }
};
#define throw_line(arg) throw my_exception(arg, __FILE__, __LINE__);

#endif
