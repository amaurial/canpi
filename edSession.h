#ifndef EDSESSION_H
#define EDSESSION_H

#define FN_SIZE 28

#include <string>
#include <log4cpp/Category.hh>
#include <time.h>

#include "utils.h"
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

        void setFnType(int fn ,byte state);
        byte getFnType(int fn);

        void setFnState(int fn ,byte state);
        byte getFnState(int fn);

        void setEdName(string edname);
        string getEdName();

        void setEdHW(string hw);
        string getEdHW();

        void setLocoName(string loconame);
        string getLocoName();

        byte getDccByte(int fn);


    protected:
    private:
        int loco;
        char sessionid;
        long last_keep_alive;
        byte direction;
        byte ad_type;
        struct timespec cbus_time;
        struct timespec ed_time;
        byte fns[FN_SIZE];
        byte fnstype[FN_SIZE];
        byte speed;
        string session_name;
        string hname;
        string loconame;
        log4cpp::Category *logger;

        byte setBit(byte val, int pos);
        byte clearBit(byte val, int pos);

};

#endif // EDSESSION_H
