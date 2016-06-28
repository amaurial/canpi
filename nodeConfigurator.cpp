#include "nodeConfigurator.h"

nodeConfigurator::nodeConfigurator(string file,log4cpp::Category *logger)
{
    this->configFile = file;
    this->logger = logger;
    loadParamsToMemory();
    nvs_set = 0;
    setNodeParams(MANU_MERG,MSOFT_MIN_VERSION,MID,0,0,getNumberOfNVs(),MSOFT_VERSION,MFLAGS);
}

nodeConfigurator::~nodeConfigurator()
{
    //dtor
}

void nodeConfigurator::printMemoryNVs(){
    int i;
    cout << "NVs: ";
    for (i=0;i<PARAMS_SIZE;i++){
        cout << int(NV[i]) << " ";
    }
    cout << endl;

    //logger->debug("Memory NVs:[%s]",NV);
}

void nodeConfigurator::setNodeParams(byte p1,byte p2, byte p3,byte p4,byte p5, byte p6, byte p7, byte p8){
    NODEPARAMS[0] = p1;
    NODEPARAMS[1] = p2;
    NODEPARAMS[2] = p3;
    NODEPARAMS[3] = p4;
    NODEPARAMS[4] = p5;
    NODEPARAMS[5] = p6;
    NODEPARAMS[6] = p7;
    NODEPARAMS[7] = p8;
}
byte nodeConfigurator::getNodeParameter(byte idx){
    //idx starts at 1
    if (idx < 8){
        return NODEPARAMS[idx-1];
    }
    return 0;
}

byte nodeConfigurator::getNV(int idx){
    int i;
    i = idx - 1;
    if (i < 0){
        i = 0;
    }
    return NV[i];
}

byte nodeConfigurator::setNV(int idx,byte val){
    int i;
    bool r;
    i = idx - 1;
    if (i < 0){
        i = 0;
    }
    NV[i] = val;
    nvs_set++;

    if (nvs_set >= PARAMS_SIZE){
        nvs_set = 0;

        logger->debug ("Received all variables. Saving to file.");
        printMemoryNVs();

        r = setSSID(nvToString(P_SSID,P5_SIZE));
        if (!r) logger->error ("Failed to save NVs SSID");

        r = setPassword(nvToString(P_PASSWD,P6_SIZE));
        if (!r) logger->error ("Failed to save NVs Password");

        r = setRouterSSID(nvToString(P_ROUTER_SSID,P7_SIZE));
        if (!r) logger->error ("Failed to save NVs Router SSID");

        r = setRouterPassword(nvToString(P_ROUTER_PASSWD,P8_SIZE));
        if (!r) logger->error ("Failed to save NVs Router password");

        r = setTcpPort(nvToInt(P_TCP_PORT,P2_SIZE));
        if (!r) logger->error ("Failed to save NVs Tcp port");

        r = setCanGridPort(nvToInt(P_GRID_TCP_PORT,P3_SIZE));
        if (!r) logger->error ("Failed to save NVs grid tcp port");

        r = setTurnoutFile(nvToString(P_TURNOUT_FILE,P10_SIZE));
        if (!r) logger->error ("Failed to save NVs turnout files");

        r = setMomentaryFn(nvToMomentary());
        if (!r) logger->error ("Failed to save NVs momentaries");

        r = setAPMode(nvToApMode());
        if (!r) logger->error ("Failed to save NV ap mode");

        r = setApChannel(NV[P_WIFI_CHANNEL]);
        if (!r) logger->error ("Failed to save Wifi channel");

        r = enableCanGrid(nvToCanGrid());
        if (!r) logger->error ("Failed to save NV enable can grid");

        int v;
        string loglevel;
        v = nvToLogLevel();
        switch (v){
        case 0:
            loglevel = TAG_INFO;
            break;
        case 1:
            loglevel = TAG_WARN;
            break;
        case 2:
            loglevel = TAG_DEBUG;
            break;
        default:
            loglevel = TAG_INFO;
        }
        r = setLogLevel(loglevel);
        if (!r) logger->error ("Failed to save NVs loglevel");
    }
    return 0;
}

void nodeConfigurator::loadParamsToMemory(){
    cout << "Loading NVs to memory" << endl;
    loadParam1();
    loadParamsInt2Bytes(getTcpPort(), P_TCP_PORT);
    loadParamsInt2Bytes(getcanGridPort(), P_GRID_TCP_PORT);
    NV[P_WIFI_CHANNEL] = getApChannel() & 0xff;
    loadParamsString(getSSID(), P_SSID, P5_SIZE);
    loadParamsString(getPassword(), P_PASSWD, P6_SIZE);
    loadParamsString(getRouterSSID(), P_ROUTER_SSID, P7_SIZE);
    loadParamsString(getRouterPassword(), P_ROUTER_PASSWD, P8_SIZE);
    loadParamsString(getServiceName(), P_SERVICE_NAME, P9_SIZE);
    loadParamsString(getTurnoutFile(), P_TURNOUT_FILE, P10_SIZE);
    momentaryFnsToNVs();
}

void nodeConfigurator::loadParam1(){
    byte p1 = 0;
    if (getAPMode()){
        cout << "AP Mode set to true" << endl;
        p1 = 1;
    }
    if (isCanGridEnabled()){
        cout << "Can grid set to true" << endl;
        p1 = p1 | 0b00000010;
    }
    string l = getLogLevel();
    if (l.compare(TAG_INFO) == 0){
        p1 = p1 | 0b00000000;
    }
    else if (l.compare(TAG_WARN) == 0){
        p1 = p1 | 0b00000100;
    }
    else if (l.compare(TAG_DEBUG) == 0){
        p1 = p1 | 0b00001000;
    }
    else{
        p1 = p1 | 0b00000011;
    }
    NV[PARAM1] = p1;
    cout << "P1 " << p1 << " " << int(NV[0]) << endl;
}

void nodeConfigurator::loadParamsInt2Bytes(int value, unsigned int idx){
    byte Hb = 0;
    byte Lb = 0;

    Lb = value & 0xff;
    Hb = (value >> 8) & 0xff;
    //little indian
    NV[idx+1] = Hb;
    NV[idx] = Lb;

    cout << "P int " << value << " " << int(NV[idx]) << " " << int(NV[idx+1]) << endl;
}

void nodeConfigurator::loadParamsString(string value, unsigned int idx, unsigned int maxsize){
    unsigned int i,ssize;

    ssize = value.size();
    if (value.size() > maxsize){
        ssize = maxsize;
    }

    for (i = 0;i < ssize; i++){
        NV[idx + i] = value.c_str()[i];
        cout <<  NV[idx + i] << " ";
    }

    //fill the rest with 0
    if (value.size() < maxsize){
        for (i=value.size() ; i < maxsize ; i++){
            NV[idx + i] = 0;
        }
    }
    cout << endl;
}

bool nodeConfigurator::saveConfig(string key,string val){
    Config cfg;
    try
    {
       cfg.readFile(configFile.c_str());

       if (cfg.exists(key)){
            libconfig::Setting &varkey = cfg.lookup(key);
            varkey = val;
            cfg.writeFile(configFile.c_str());
       }
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "File I/O error" << std::endl;
        std::cout << "File I/O error" << std::endl;
        return false;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
        std::cout << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
        return false;
    }
    return true;
}
bool nodeConfigurator::saveConfig(string key,int val){
    Config cfg;
    try
    {
       cfg.readFile(configFile.c_str());

       if (cfg.exists(key)){
            libconfig::Setting &varkey = cfg.lookup(key);
            varkey = val;
            cfg.writeFile(configFile.c_str());
       }
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "File I/O error" << std::endl;
        std::cout << "File I/O error" << std::endl;
        return false;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
        std::cout << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
        return false;
    }
    return true;
}

string nodeConfigurator::getStringConfig(string key)
{
    Config cfg;
    string ret;
    ret = "";
    try
    {
       cfg.readFile(configFile.c_str());
       if (cfg.exists(key)){
            cfg.lookupValue(key,ret);
       }
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "File I/O error" << std::endl;
        std::cout << "File I/O error" << std::endl;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
        std::cout << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
    }
    catch(const SettingNotFoundException &nfex)
    {
        std::cerr << "Key not found" << std::endl;
        std::cout << "Key not found" << std::endl;
    }
    return ret;

}
/**
* Usefull function to get integer from config file
**/
int nodeConfigurator::getIntConfig(string key)
{
    int ret;
    Config cfg;
    ret = INTERROR;
    try
    {
       cfg.readFile(configFile.c_str());
       if (cfg.exists(key)){
            bool r = cfg.lookupValue(key,ret);
            if (!r) ret = INTERROR;
       }
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "File I/O error" << std::endl;
        std::cout << "File I/O error" << std::endl;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
        std::cout << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
    }
    catch(const SettingNotFoundException &nfex)
    {
        std::cerr << "Key not found" << std::endl;
        std::cout << "Key not found" << std::endl;
    }
    return ret;
}

string nodeConfigurator::getNodeName(){
    return string(NODE_NAME);
}

//saves the string to config file
string nodeConfigurator::getMomentaryFn(){
    string ret;
    ret = getStringConfig(TAG_FNMOM);
    return ret;
}
//gets the string to config file
bool nodeConfigurator::setMomentaryFn(string val){
    return saveConfig(TAG_FNMOM,val);
}
//transform the bits in the array to the momentary string
//number comma separated
string nodeConfigurator::nvToMomentary(){
    int i;
    byte a,fn,j;
    fn = 0;
    stringstream ss;
    string fns;

    for (i = 0; i < P11_SIZE ;i++){
        a = NV[i+P_MOMENTARY_FNS];
        for (j = 0; j < 8; j++){
            if ((( a>>(7-j) ) & 0x01) == 1){
                ss << int(fn);
                ss << ",";
            }
            fn++;
        }
    }
    fns = ss.str();
    if (fns.size() > 0){
        //delete the last commma
        fns = fns.substr(0,fns.size()-1);
    }
    return fns;
}

void nodeConfigurator::momentaryFnsToNVs(){
    string val;
    int i;
    byte t;
    char fns[P11_SIZE];
    int idx,ibyte;

    memset(fns,0,P11_SIZE);

    val = getMomentaryFn();

    if (val.size() > 0){
        vector <string> vals;
        vals = split(val,',',vals);

        logger->debug("Config: loading fns:%s size:%d to NVs",val.c_str(),vals.size());

        if (!vals.empty()){
            for (auto s:vals){
                i = atoi(s.c_str());
                logger->debug("Config - Set Fn %d to momentary", i);
                if (i < FN_SIZE){
                    if (i < 8) ibyte = 0;
                    if (i > 7 && i < 16) ibyte = 1;
                    if (i > 15 && i < 24) ibyte = 2;
                    if (i > 23 && i < 32) ibyte = 3;
                    if (i > 31) return;
                    t = 1;
                    idx = 7 - (i - ibyte*8);
                    fns[ibyte] = fns[ibyte] | t << idx;
                }
            }
            logger->debug("FN momentary bytes %02x %02x %02x %02x",fns[0],fns[1],fns[2],fns[3]);
            //copy to memory
            for (i=0; i< P11_SIZE; i++){
                NV[i+P_MOMENTARY_FNS]= fns[i];
            }
        }
    }
}

vector<string> & nodeConfigurator::split(const string &s, char delim, vector<string> &elems)
{
    stringstream ss(s+' ');
    string item;
    while(getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

//general function that gets an array index and size and get the string
string nodeConfigurator::nvToString(int index,int slen){
    stringstream ss;
    int i;
    for (i=0; i < slen;i++){
        ss << NV[index + i];
    }
    return ss.str();
}

//general function that gets an array index and size and get the integer
// the first byte is the highest byte
int nodeConfigurator::nvToInt(int index,int slen){
    int val;
    int i;
    val = 0;
    for (i=0; i < slen ;i++){
        //val = val << 8;
        val = val | NV[index + i] << 8*i;
    }
    return val;
}

bool nodeConfigurator::nvToApMode(){

    if ((NV[PARAM1] & 0x01) == 1){
        return true;
    }
    return false;
}

bool nodeConfigurator::nvToCanGrid(){

    if ((NV[PARAM1] & 0x02) == 2){
        return true;
    }
    return false;
}

int nodeConfigurator::nvToLogLevel(){
    return (NV[PARAM1] & 0x0C)>>2;
}

bool nodeConfigurator::setTcpPort(int val){
    return saveConfig(TAG_TCP_PORT,val);
}

int nodeConfigurator::getTcpPort(){
    int ret;
    ret = getIntConfig(TAG_TCP_PORT);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_TCP_PORT);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get tcp port. Default is 30" << endl;
        }
        ret = 30;
    }
    return ret;
}

int nodeConfigurator::getcanGridPort(){
    int ret;
    ret = getIntConfig(TAG_GRID_PORT);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_GRID_PORT);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the grid tcp port. Default is 31" << endl;
        }
        ret = 31;
    }
    return ret;
}
bool nodeConfigurator::setCanGridPort(int val){
    return saveConfig(TAG_GRID_PORT,val);
}

int nodeConfigurator::getCanID(){
    int ret;
    ret = getIntConfig(TAG_CANID);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_CANID);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the canid. Default is 110" << endl;
        }
        ret = 110;
    }
    return ret;
}
bool nodeConfigurator::setCanID(int val){
    return saveConfig(TAG_CANID,val);
}

int nodeConfigurator::getNodeNumber(){
    int ret;
    ret = getIntConfig(TAG_NN);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_NN);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the node_number. Default is 4321" << endl;
        }
        ret = 4321;
    }
    return ret;
}
bool nodeConfigurator::setNodeNumber(int val){
    return saveConfig(TAG_NN,val);
}

bool nodeConfigurator::getAPMode(){
    string ret;
    ret = getStringConfig(TAG_AP_MODE);
    if (ret.empty()){
        cout << "Failed to get ap_mode . Default is false" << endl;
        return false;
    }
    if ((ret.compare("true") == 0) | (ret.compare("TRUE") == 0) | (ret.compare("True") == 0)){
        return true;
    }
    return false;
}
bool nodeConfigurator::setAPMode(bool apmode){
    if (apmode){
        return saveConfig(TAG_AP_MODE,"True");
    }
    else{
        return saveConfig(TAG_AP_MODE,"False");
    }
}

bool nodeConfigurator::isCanGridEnabled(){
    string ret;
    ret = getStringConfig(TAG_CAN_GRID);
    if (ret.empty()){
        cout << "Failed to get can_grid . Default is false" << endl;
        return false;
    }
    if ((ret.compare("true") == 0) | (ret.compare("TRUE") == 0) | (ret.compare("True") == 0)){
        return true;
    }
    return false;
}
bool nodeConfigurator::enableCanGrid(bool grid){
    if (grid){
        return saveConfig(TAG_CAN_GRID,"True");
    }
    else{
        return saveConfig(TAG_CAN_GRID,"False");
    }
}

bool nodeConfigurator::setSSID(string val){
    return saveConfig(TAG_SSID,val);
}
string nodeConfigurator::getSSID(){
    string ret;
    ret = getStringConfig(TAG_SSID);
    return ret;
}

bool nodeConfigurator::setPassword(string val){
    return saveConfig(TAG_PASSWD,val);
}
string nodeConfigurator::getPassword(){
    string ret;
    ret = getStringConfig(TAG_PASSWD);
    return ret;
}

bool nodeConfigurator::setRouterSSID(string val){
    return saveConfig(TAG_ROUTER_SSID,val);
}

string nodeConfigurator::getRouterSSID(){
    string ret;
    ret = getStringConfig(TAG_ROUTER_SSID);
    return ret;
}

bool nodeConfigurator::setRouterPassword(string val){
    return saveConfig(TAG_ROUTER_PASSWD,val);
}
string nodeConfigurator::getRouterPassword(){
    string ret;
    ret = getStringConfig(TAG_ROUTER_PASSWD);
    return ret;
}

bool nodeConfigurator::setLogLevel(string val){
    return saveConfig(TAG_LOGLEVEL,val);
}
string nodeConfigurator::getLogLevel(){
    string ret;
    ret = getStringConfig(TAG_LOGLEVEL);
    if (ret.empty()){
        cout << "Failed to get log level name. Default is WARN" << endl;
        ret = "WARN";
    }
    return ret;
}

bool nodeConfigurator::setLogFile(string val){
    return saveConfig(TAG_LOGFILE,val);
}
string nodeConfigurator::getLogFile(){
    string ret;
    ret = getStringConfig(TAG_LOGFILE);
    if (ret.empty()){
        cout << "Failed to get log file name. Default is canpi.log" << endl;
        ret = "canpi.log";
    }
    return ret;
}

bool nodeConfigurator::setServiceName(string val){
    return saveConfig(TAG_SERV_NAME,val);
}
string nodeConfigurator::getServiceName(){
    string ret;
    ret = getStringConfig(TAG_SERV_NAME);
    return ret;
}

bool nodeConfigurator::setLogAppend(bool val){
    if (val){
        return saveConfig(TAG_LOGAPPEND,"True");
    }
    else{
        return saveConfig(TAG_LOGAPPEND,"False");
    }
}
bool nodeConfigurator::getLogAppend(){
    string ret;
    ret = getStringConfig(TAG_LOGAPPEND);
    if (ret.empty()){
        cout << "Failed to get logappend . Default is false" << endl;
        return false;
    }
    if ((ret.compare("true") == 0) | (ret.compare("TRUE") == 0) | (ret.compare("True") == 0)){
        return true;
    }
    return false;
}

bool nodeConfigurator::setTurnoutFile(string val){
    return saveConfig(TAG_TURNOUT,val);
}
string nodeConfigurator::getTurnoutFile(){
    string ret;
    ret = getStringConfig(TAG_TURNOUT);
    if (ret.empty()){
        cout << "Failed to get turnout file name. Defaul is turnout.txt" << endl;
        ret = "turnout.txt";
    }
    return ret;
}

bool nodeConfigurator::setCanDevice(string val){
    return saveConfig(TAG_CANDEVICE,val);
}
string nodeConfigurator::getCanDevice(){
    string ret;
    ret = getStringConfig(TAG_CANDEVICE);
    if (ret.empty()){
        cout << "Failed to get the can device. Default is can0" << endl;
        ret = "can0";
    }
    return ret;
}

int nodeConfigurator::getApChannel(){
    int ret;
    ret = getIntConfig(TAG_APCHANNEL);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_APCHANNEL);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the ap_channel. Default is 6" << endl;
        }
        ret = 6;
    }
    return ret;
}
bool nodeConfigurator::setApChannel(int val){
    return saveConfig(TAG_APCHANNEL,val);
}

string nodeConfigurator::getConfigFile(){
    return configFile;
}
void nodeConfigurator::setConfigFile(string val){
    configFile = val;
}

int nodeConfigurator::getPB(){
    int ret;
    ret = getIntConfig(TAG_BP);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_BP);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the button_pin. Default is 4" << endl;
        }
        ret = 4;
    }
    return ret;
}
bool nodeConfigurator::setPB(int val){
    return saveConfig(TAG_BP,val);
}

int nodeConfigurator::getGreenLed(){
    int ret;
    ret = getIntConfig(TAG_GL);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_GL);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the green_led_pin. Default is 5" << endl;
        }
        ret = 5;
    }
    return ret;
}
bool nodeConfigurator::setGreenLed(int val){
    return saveConfig(TAG_GL,val);
}

int nodeConfigurator::getYellowLed(){
    int ret;
    ret = getIntConfig(TAG_YL);
    if (ret == INTERROR){
        string r = getStringConfig(TAG_YL);
        if (r.size() > 0){
            //try to convert
            try{
                ret = atoi(r.c_str());
            }
            catch(...){
                cout << "Failed to convert " << r << " to int" << endl;
            }
        }
        else{
            cout << "Failed to get the yellow_led_pin. Default is 6" << endl;
        }
        ret = 6;
    }
    return ret;
}
bool nodeConfigurator::setYellowLed(int val){
    return saveConfig(TAG_YL,val);
}
