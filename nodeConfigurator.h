#ifndef NODECONFIGURATOR_H
#define NODECONFIGURATOR_H

#include <iostream>
#include <string>
#include "libconfig.h++"
#include "utils.h"
#include "opcodes.h"

using namespace std;

class nodeConfigurator
{
    public:
        nodeConfigurator(string file);
        virtual ~nodeConfigurator();

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

        string configFile;
        Config cfg;
};

#endif // NODECONFIGURATOR_H
