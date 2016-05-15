#include "edSession.h"

edSession::edSession(log4cpp::Category *logger)
{
    //ctor
    loco = -1;
    sessionid = 0;
    for (int i=0;i<FN_SIZE;i++){
        fns[i]=0;
        fnstype[i]=0;
    }

    fnstype[0] = 1;// #light is toggle
    //cbus_time = 0;
    //ed_time = 0;
    speed = 0;
    direction = 0;
    ad_type = 'S';
    this->logger = logger;
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

void edSession::setSpeed(byte val){
    this->speed = val;
}
byte edSession::getSpeed(){
    return speed;
}

void edSession::setSession(byte session){
    this->sessionid = session;
}
byte edSession::getSession(){
    return sessionid;
}

void edSession::setDirection(byte direction){
    this->direction = direction;
}
byte edSession::getDirection(){
    return direction;
}

void edSession::setAddressType(byte val){
    this->ad_type = val;
}
byte edSession::getAddressType(){
    return ad_type;
}

void edSession::setCbusTime(struct timespec val){
    this->cbus_time = val;
}
struct timespec edSession::getCbusTime(){
    return cbus_time;
}

void edSession::setEDTime(struct timespec val){
    this->ed_time = val;
}
struct timespec edSession::getEDTime(){
    return ed_time;
}

void edSession::setEdName(string edname){
    this->session_name = edname;
}

string edSession::getEdName(){
    return session_name;
}

void edSession::setEdHW(string hw){
    this->hname = hw;
}

string edSession::getEdHW(){
    return hname;
}

void edSession::setLocoName(string loconame){
    this->loconame = loconame;
}

string edSession::getLocoName(){
    return loconame;
}

void edSession::setFnType(int fn ,byte state){
    if ((state != 0) & (state != 1)) {
        //logger->debug("Invalid type %d",state);
        return;
    }
    //logger->debug("Set type to %d",state);
    fnstype[fn] = state;
}

byte edSession::getFnType(int fn){
    return fnstype[fn];
}

void edSession::setFnState(int fn ,byte state){
    if ((state != 0) & (state != 1)) {
        //logger->debug("Invalid state %d",state);
        return;
    }
    //logger->debug("Set state to %d",state);
    fns[fn] = state;
}

byte edSession::getFnState(int fn){
    return fns[fn];
}

byte edSession::setBit(byte val, int pos){
    if (pos > 8) return val;
    return val | (1<<pos);
}
byte edSession::clearBit(byte val, int pos){
    if (pos > 8) return val;
    return val & ~(1<<pos);
}

byte edSession::getDccByte(int fn){
    /*
    # create the byte for the DCC
    #1 is F0(FL) to F4
    #2 is F5 to F8
    #3 is F9 to F12
    #4 is F13 to F19
    #5 is F20 to F28
    # see http://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf
    */
    byte fnbyte = 0x00;
    int i = 0;
    int j = 0;

    if ((fn >=0) & (fn < 5)){
        i=0;j=5;
    }
    if ((4 < fn) & (fn < 9)){
        i=5;j=9;
    }
    if ((8 < fn) & (fn < 13 )){
        i=9;j=13;
    }
    if ((12 < fn) & (fn < 20 )){
        i=13;j=20;
    }
    if ((19 < fn) & (fn < 29)){
        i=20;j=29;
    }
    if ((i == 0) & (j == 0)) return -1;

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
