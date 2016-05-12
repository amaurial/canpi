#include "edSession.h"

edSession::edSession()
{
    //ctor
    loco = -1;
    sessionid = 0;
    for (int i=0;i<FN_SIZE;i++){
        fns[i]=0;
        fnstype[i]=0;
    }

    fnstype[0] = 1;// #light is toggle
}

edSession::~edSession()
{
    //dtor
}

void edSession::setLoco(int loco){
    this->loco = loco;
}
int edSession::getLoco(){
    return loco;
}

void edSession::setSpeed(char val){
    this->speed = val;
}
char edSession::getSpeed(){
    return speed;
}

void edSession::setSession(char session){
    this->sessionid = session;
}
char edSession::getSession(){
    return sessionid;
}

void edSession::setDirection(char direction){
    this->direction = direction;
}
char edSession::getDirection(){
    return direction;
}

void edSession::setAddressType(char val){
    this->ad_type = val;
}
char edSession::getAddressType(){
    return ad_type;
}

void edSession::setCbusTime(long val){
    this->cbus_time = val;
}
long edSession::getCbusTime(){
    return cbus_time;
}

void edSession::setEDTime(long val){
    this->ed_time = val;
}
long edSession::getEDTime(){
    return ed_time;
}

char edSession::setBit(char val, int pos){
    if (pos > 8) return val;
    return val | (1<<pos);
}
char edSession::clearBit(char val, int pos){
    if (pos > 8) return val;
    return val & ~(1<<pos);
}

char edSession::getDccByte(int fn){
    /*
    # create the byte for the DCC
    #1 is F0(FL) to F4
    #2 is F5 to F8
    #3 is F9 to F12
    #4 is F13 to F19
    #5 is F20 to F28
    # see http://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf
    */
    char fnbyte = 0x00;
    int i = 0;
    int j = 0;

    if (fn >=0 && fn < 5){
        i=0;j=5;
    }
    if (4 < fn && fn < 9){
        i=5;j=9;
    }
    if (8 < fn && fn < 13 ){
        i=9;j=13;
    }
    if (12 < fn && fn < 20 ){
        i=13;j=20;
    }
    if (19 < fn and fn < 29){
        i=20;j=29;
    }
    if (i == 0 && j == 0) return -1;

    int k = 0;

    for (int f = i;f < j; f++){
        if (fns[f]== 1){
            if (f == 0){// #special case: light
                fnbyte = setBit(fnbyte, 4);
            }
            else{
                fnbyte = setBit(fnbyte, k);
            }
        }
        if (f != 0) k++;
    }
    return fnbyte;
}
