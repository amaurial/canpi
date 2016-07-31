#include "edSession.h"
#include "nodeConfigurator.h"

edSession::edSession(log4cpp::Category *logger)
{
    //ctor
    loco = -1;
    sessionid = 0;
    for (int i=0;i<FN_SIZE;i++){
        fns[i]=FNState::OFF;
        fnstype[i]=FNType::SWITCH;
    }
    fnstype[2] = FNType::MOMENTARY;// #horn is momentary
    speed = 0;
    direction = 0;
    ad_type = 'S';
    sessionType = SessionType::MULTI_SESSION;
    this->logger = logger;
    //set inital time
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME,&spec);
    setCbusTime(spec);
    setEDTime(spec);
}

edSession::~edSession()
{
    //dtor
}

void edSession::configChanged(){
    getMomentaryFNs();
    getMomentaryFNs(this->getLoco());
}
//generic configuration
void edSession::getMomentaryFNs(){
    string val;
    val = config->getMomentaryFn();
    setMomentaryFNs(val);
}
//specific for each loco
void edSession::getMomentaryFNs(int loco){
    stringstream ss;
    string s;
    ss << "R";
    ss << loco;
    s = ss.str();
    logger->debug("[edSession] Get fn config for loco %s", s.c_str());
    if (!config->existConfigEntry(s)){
        logger->debug("[edSession] No momentary FNs found");
        return;
    }
    logger->debug("[edSession] Fn config exists");
    string val = config->getPairValue(s);
    logger->debug("[edSession] Fn config is %s", val.c_str());
    setMomentaryFNs(val);
}

void edSession::setMomentaryFNs(string val){
    int i;

    logger->debug("[edSession] FNs %s", val.c_str());
    if (val.size() > 0){
        vector <string> vals;
        vals = split(val,',',vals);
        if (!vals.empty()){
            logger->debug("[edSession] Reset the Fns to toggle");
            for (i=0;i<FN_SIZE;i++){
                fnstype[i]=FNType::SWITCH;
            }
            for (auto s:vals){
                logger->debug("[edSession] Set Fn %s to momentary", s.c_str());
                i = atoi(s.c_str());
                if (i < FN_SIZE){
                    setFnType(i, FNType::MOMENTARY);
                }
            }
        }
    }
    else logger->debug("[edSession] No momentary FNs found");
}

string edSession::getMomentary(){
    int i;
    stringstream ss;
    string s;

    for (i=0;i<FN_SIZE;i++){
        if (fnstype[i] == FNType::MOMENTARY){
            ss << i;
            ss << ",";
        }
    }
    s = ss.str();
    if (s.size() > 0){
        s = s.substr(0, s.size() - 1);
    }
    return s;
}

vector<string> & edSession::split(const string &s, char delim, vector<string> &elems)
{
    stringstream ss(s+' ');
    string item;
    while(getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

void edSession::setNodeConfigurator(nodeConfigurator *config){
    this->config = config;
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

void edSession::setSessionType(SessionType stype){
    sessionType = stype;
}
SessionType edSession::getSessionType(){
    return sessionType;
}

char edSession::getCharSessionType(){
    if (sessionType == SessionType::MULTI_SESSION) return 'S';
    if (sessionType == SessionType::MULTI_THROTTLE) return 'T';
    return 'T';
}

void edSession::setFnType(int fn ,FNType state){
    fnstype[fn] = state;
}

FNType edSession::getFnType(int fn){
    return fnstype[fn];
}

void edSession::setFnState(int fn ,FNState state){
    fns[fn] = state;
}

FNState edSession::getFnState(int fn){
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
