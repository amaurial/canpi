#ifndef EDSESSION_H
#define EDSESSION_H

#define FN_SIZE 28

class edSession
{
    public:
        edSession();
        virtual ~edSession();

        void setLoco(int loco);
        int getLoco();

        void setSession(char session);
        char getSession();

        void setSpeed(char val);
        char getSpeed();

        void setDirection(char direction);
        char getDirection();

        void setAddressType(char val);
        char getAddressType();

        void setCbusTime(long val);
        long getCbusTime();

        void setEDTime(long val);
        long getEDTime();

        char getDccByte(int fn);


    protected:
    private:
        int loco;
        char sessionid;
        long last_keep_alive;
        char direction;
        char ad_type;
        long cbus_time;
        long ed_time;
        char fns[FN_SIZE];
        char fnstype[FN_SIZE];
        char speed;

        char setBit(char val, int pos);
        char clearBit(char val, int pos);

};

#endif // EDSESSION_H
