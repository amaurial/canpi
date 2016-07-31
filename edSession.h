#ifndef EDSESSION_H
#define EDSESSION_H

#include <string>
#include <log4cpp/Category.hh>
#include <time.h>

#include "utils.h"
#include "nodeConfigurator.h"

using namespace std;

class edSession
{
    public:
        edSession(log4cpp::Category *logger);
        virtual ~edSession();

        void setLoco(int loco);
        int getLoco();

        void setSession(byte session);
        byte getSession();

        void setSpeed(byte val);
        byte getSpeed();

        void setDirection(byte direction);
        byte getDirection();

        void setAddressType(byte val);
        byte getAddressType();

        void setCbusTime(struct timespec val);
        struct timespec getCbusTime();

        void setEDTime(struct timespec val);
        struct timespec getEDTime();

        void setFnType(int fn ,FNType state);
        FNType getFnType(int fn);

        void setFnState(int fn ,FNState state);
        FNState getFnState(int fn);

        void setEdName(string edname);
        string getEdName();

        void setEdHW(string hw);
        string getEdHW();

        void setLocoName(string loconame);
        string getLocoName();

        void setNodeConfigurator(nodeConfigurator *config);

        byte getDccByte(int fn);

        void configChanged();
        void getMomentaryFNs();
        void getMomentaryFNs(int loco);
        string getMomentary();

        void setSessionType(SessionType stype);
        SessionType getSessionType();

        char getCharSessionType();

    protected:
    private:
        int loco;
        char sessionid;
        long last_keep_alive;
        byte direction;
        byte ad_type;
        struct timespec cbus_time;
        struct timespec ed_time;
        FNState fns[FN_SIZE];
        FNType fnstype[FN_SIZE];
        byte speed;
        string session_name;
        string hname;
        string loconame;
        log4cpp::Category *logger;
        nodeConfigurator *config;
        SessionType sessionType;

        byte setBit(byte val, int pos);
        byte clearBit(byte val, int pos);
        void setMomentaryFNs(string val);

        vector<string> & split(const string &s, char delim, vector<string> &elems);
};

#endif // EDSESSION_H
