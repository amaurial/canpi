#ifndef NODECONFIGURATOR_H
#define NODECONFIGURATOR_H

#include <iostream>
#include <string>
#include <vector>
#include "libconfig.h++"
#include "utils.h"
#include "opcodes.h"

/**
1 byte
apmode bit 1, enable can grid bit 2, log level bit 3,4

2 bytes
tcp port

2 bytes
grid tcp port

1 byte
wifi channel

8 bytes
ssid

8 bytes
ssid password

8 bytes
router ssid

8 bytes
router password

8 bytes
service name

11 bytes
turnout file name
**/
#define P1_SIZE 1 //apmode bit 1, enable can grid bit 2, log level bit 3,4
#define P2_SIZE 2 //tcp port
#define P3_SIZE 2 //grid tcp port
#define P4_SIZE 1 //wifi channel
#define P5_SIZE 8 //ssid
#define P6_SIZE 8 //ssid password
#define P7_SIZE 8 //router ssid
#define P8_SIZE 8 //router ssid password
#define P9_SIZE 8 //service name
#define P10_SIZE 11 //turnout file name

#define PARAMS_SIZE         P1_SIZE + P2_SIZE + P3_SIZE + P4_SIZE + P5_SIZE + P6_SIZE + P7_SIZE + P8_SIZE + P9_SIZE + P10_SIZE

#define PARAM1 0 //apmode bit 1, enable can grid bit 2, log level bit 3,4
#define P_TCP_PORT           PARAM1 + P1_SIZE
#define P_GRID_TCP_PORT      P_TCP_PORT + P2_SIZE
#define P_WIFI_CHANNEL       P_GRID_TCP_PORT + P3_SIZE
#define P_SSID               P_WIFI_CHANNEL + P4_SIZE
#define P_PASSWD             P_SSID + P5_SIZE
#define P_ROUTER_SSID        P_PASSWD + P6_SIZE
#define P_ROUTER_PASSWD      P_ROUTER_SSID + P7_SIZE
#define P_SERVICE_NAME       P_ROUTER_PASSWD + P8_SIZE
#define P_TURNOUT_FILE       P_SERVICE_NAME + P9_SIZE


using namespace std;

class nodeConfigurator
{
    public:
        nodeConfigurator(string file);
        virtual ~nodeConfigurator();

        byte getParameter(int idx);
        byte setParameter(int idx,byte val);

        string getNodeName();

        int getTcpPort();
        bool setTcpPort(int port);

        int getcanGridPort();
        bool setCanGridPort(int port);

        int getCanID();
        bool setCanID(int canid);

        int getNodeNumber();
        bool setNodeNumber(int nn);

        bool getAPMode();
        bool setAPMode(bool apmode);

        bool isCanGridEnabled();
        bool enableCanGrid(bool grid);

        bool setSSID(string val);
        string getSSID();

        bool setPassword(string val);
        string getPassword();

        bool setRouterSSID(string val);
        string getRouterSSID();

        bool setRouterPassword(string val);
        string getRouterPassword();

        bool setLogLevel(string val);
        string getLogLevel();

        bool setLogFile(string val);
        string getLogFile();

        bool setLogAppend(bool val);
        bool getLogAppend();

        bool setTurnoutFile(string val);
        string getTurnoutFile();

        bool setCanDevice(string val);
        string getCanDevice();

        bool setServiceName(string val);
        string getServiceName();

        int getApChannel();
        bool setApChannel(int channel);

        string getConfigFile();
        void setConfigFile(string val);

        int getPB();
        bool setPB(int val);

        int getGreenLed();
        bool setGreenLed(int val);

        int getYellowLed();
        bool setYellowLed(int val);

        string getStringConfig(string key);
        int getIntConfig(string key);
    protected:
    private:
        bool saveConfig(string key,string val);
        bool saveConfig(string key,int val);
        void startIndexParams();
        void loadParamsToMemory();
        void loadParam1();
        void loadParamsInt2Bytes();

        string configFile;
        Config cfg;
        char PARAMS[PARAMS_SIZE];
        vector<pair<int,int>> param_index; //the pair is the start position in the array and the parameter length
};

#endif // NODECONFIGURATOR_H
