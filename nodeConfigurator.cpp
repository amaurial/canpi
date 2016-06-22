#include "nodeConfigurator.h"

nodeConfigurator::nodeConfigurator(string file,log4cpp::Category *logger)
{
    this->configFile = file;
    this->logger = logger;
    loadParamsToMemory();
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
    i = idx - 1;
    if (i < 0){
        i = 0;
    }
    NV[i] = val;
    //TODO need to discover when to save a parameter
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
}

void nodeConfigurator::loadParam1(){
    byte p1 = 0;
    if (getAPMode()){
        p1 = 1;
    }
    if (isCanGridEnabled()){
        p1 = p1 | 0b00000011;
    }
    string l = getLogLevel();
    if (l.compare("INFO") == 0){
        p1 = p1 | 0b00000011;
    }
    else if (l.compare("WARN") == 0){
        p1 = p1 | 0b00000111;
    }
    else if (l.compare("DEBUG") == 0){
        p1 = p1 | 0b00001011;
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

    NV[idx] = Hb;
    NV[idx+1] = Lb;

    cout << "P int " << value << " " << int(NV[idx]) << " " << int(NV[idx+1]) << endl;

}

void nodeConfigurator::loadParamsString(string value, unsigned int idx, unsigned int maxsize){
    unsigned i,ssize;

    ssize = value.size();
    if (value.size() > maxsize){
        ssize = maxsize;
    }

    for (int i = 0;i < ssize; i++){
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


void nodeConfigurator::startIndexParams(){
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
/*
    param_index.push_back({1,1});//apmode bit 1, enable can grid bit 2, log level bit 3,4
    param_index.push_back({2,2});//tcp port
    param_index.push_back({3,2});//grid tcp port
    param_index.push_back({4,1});//wifi channel
    param_index.push_back({5,8});//ssid
    param_index.push_back({6,8});//password
    param_index.push_back({7,8});//router ssid
    param_index.push_back({8,8});//router password
    param_index.push_back({9,8});//service name
    param_index.push_back({10,11});//turnout file name
*/
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
        return false;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
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
        return false;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
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
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
    }
    catch(const SettingNotFoundException &nfex)
    {
        std::cerr << "Key not found" << std::endl;
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
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
    }
    catch(const SettingNotFoundException &nfex)
    {
        std::cerr << "Key not found" << std::endl;
    }
    return ret;
}

string nodeConfigurator::getNodeName(){
    return string("CANWIFI");
}

int nodeConfigurator::getTcpPort(){
    int ret;
    ret = getIntConfig("tcpport");
    if (ret == INTERROR){
        cout << "Failed to get tcp port. Default is 30" << endl;
        ret = 31;
    }
    return ret;
}
bool nodeConfigurator::setTcpPort(int val){
    return saveConfig("tcpport",val);
}

int nodeConfigurator::getcanGridPort(){
    int ret;
    ret = getIntConfig("cangrid_port");
    if (ret == INTERROR){
        cout << "Failed to get the grid tcp port. Default is 31" << endl;
        ret = 31;
    }
    return ret;
}
bool nodeConfigurator::setCanGridPort(int val){
    return saveConfig("cangrid_port",val);
}

int nodeConfigurator::getCanID(){
    int ret;
    ret = getIntConfig("canid");
    if (ret == INTERROR){
        cout << "Failed to get the canid. Default is 110" << endl;
        ret = 110;
    }
    return ret;
}
bool nodeConfigurator::setCanID(int val){
    return saveConfig("canid",val);
}

int nodeConfigurator::getNodeNumber(){
    int ret;
    ret = getIntConfig("node_number");
    if (ret == INTERROR){
        cout << "Failed to get the node_number. Default is 4321" << endl;
        ret = 4321;
    }
    return ret;
}
bool nodeConfigurator::setNodeNumber(int val){
    return saveConfig("node_number",val);
}

bool nodeConfigurator::getAPMode(){
    string ret;
    ret = getStringConfig("ap_mode");
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
        return saveConfig("ap_mode","True");
    }
    else{
        return saveConfig("ap_mode","False");
    }
}

bool nodeConfigurator::isCanGridEnabled(){
    string ret;
    ret = getStringConfig("can_grid");
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
        return saveConfig("can_grid","True");
    }
    else{
        return saveConfig("can_grid","False");
    }
}

bool nodeConfigurator::setSSID(string val){
    return saveConfig("ap_ssid",val);
}
string nodeConfigurator::getSSID(){
    string ret;
    ret = getStringConfig("ap_ssid");
    return ret;
}

bool nodeConfigurator::setPassword(string val){
    return saveConfig("ap_password",val);
}
string nodeConfigurator::getPassword(){
    string ret;
    ret = getStringConfig("ap_password");
    return ret;
}

bool nodeConfigurator::setRouterSSID(string val){
    return saveConfig("router_ssid",val);
}

string nodeConfigurator::getRouterSSID(){
    string ret;
    ret = getStringConfig("router_ssid");
    return ret;
}

bool nodeConfigurator::setRouterPassword(string val){
    return saveConfig("router_password",val);
}
string nodeConfigurator::getRouterPassword(){
    string ret;
    ret = getStringConfig("router_password");
    return ret;
}

bool nodeConfigurator::setLogLevel(string val){
    return saveConfig("loglevel",val);
}
string nodeConfigurator::getLogLevel(){
    string ret;
    ret = getStringConfig("loglevel");
    if (ret.empty()){
        cout << "Failed to get log level name. Default is WARN" << endl;
        ret = "WARN";
    }
    return ret;
}

bool nodeConfigurator::setLogFile(string val){
    return saveConfig("logfile",val);
}
string nodeConfigurator::getLogFile(){
    string ret;
    ret = getStringConfig("logfile");
    if (ret.empty()){
        cout << "Failed to get log file name. Default is canpi.log" << endl;
        ret = "canpi.log";
    }
    return ret;
}

bool nodeConfigurator::setServiceName(string val){
    return saveConfig("service_name",val);
}
string nodeConfigurator::getServiceName(){
    string ret;
    ret = getStringConfig("service_name");
    return ret;
}

bool nodeConfigurator::setLogAppend(bool val){
    if (val){
        return saveConfig("logappend","True");
    }
    else{
        return saveConfig("logappend","False");
    }
}
bool nodeConfigurator::getLogAppend(){
    string ret;
    ret = getStringConfig("logappend");
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
    return saveConfig("turnout_file",val);
}
string nodeConfigurator::getTurnoutFile(){
    string ret;
    ret = getStringConfig("turnout_file");
    if (ret.empty()){
        cout << "Failed to get turnout file name. Defaul is turnout.txt" << endl;
        ret = "turnout.txt";
    }
    return ret;
}

bool nodeConfigurator::setCanDevice(string val){
    return saveConfig("candevice",val);
}
string nodeConfigurator::getCanDevice(){
    string ret;
    ret = getStringConfig("candevice");
    if (ret.empty()){
        cout << "Failed to get the can device. Default is can0" << endl;
        ret = "can0";
    }
    return ret;
}

int nodeConfigurator::getApChannel(){
    int ret;
    ret = getIntConfig("ap_channel");
    if (ret == INTERROR){
        cout << "Failed to get the ap_channel. Default is 6" << endl;
        ret = 6;
    }
    return ret;
}
bool nodeConfigurator::setApChannel(int val){
    return saveConfig("ap_channel",val);
}

string nodeConfigurator::getConfigFile(){
    return configFile;
}
void nodeConfigurator::setConfigFile(string val){
    configFile = val;
}

int nodeConfigurator::getPB(){
    int ret;
    ret = getIntConfig("button_pin");
    if (ret == INTERROR){
        cout << "Failed to get the button_pin. Default is 4" << endl;
        ret = 4;
    }
    return ret;
}
bool nodeConfigurator::setPB(int val){
    return saveConfig("button_pin",val);
}

int nodeConfigurator::getGreenLed(){
    int ret;
    ret = getIntConfig("green_led_pin");
    if (ret == INTERROR){
        cout << "Failed to get the green_led_pin. Default is 5" << endl;
        ret = 5;
    }
    return ret;
}
bool nodeConfigurator::setGreenLed(int val){
    return saveConfig("green_led_pin",val);
}

int nodeConfigurator::getYellowLed(){
    int ret;
    ret = getIntConfig("yellow_led_pin");
    if (ret == INTERROR){
        cout << "Failed to get the yellow_led_pin. Default is 6" << endl;
        ret = 6;
    }
    return ret;
}
bool nodeConfigurator::setYellowLed(int val){
    return saveConfig("yellow_led_pin",val);
}
