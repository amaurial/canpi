#ifndef NODECONFIGURATOR_H
#define NODECONFIGURATOR_H

#include <iostream>
#include <string>
#include <vector>
#include "libconfig.h++"
#include "utils.h"
#include "opcodes.h"

#define PARAMS_SIZE 60

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

        string configFile;
        Config cfg;
        char PARAMS[PARAMS_SIZE];
        vector<pair<int,int>> param_index; //the pair is the start position in the array and the parameter length
};

#endif // NODECONFIGURATOR_H
