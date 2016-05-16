#include "tcpClient.h"
#include <stdio.h>

tcpClient::tcpClient(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr,int id)
{
    //ctor

    this->server = server;
    this->can = can;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    logger->debug("Client %d created", id);
    edsession = new edSession(logger);
    this->re_speed = regex(RE_SPEED);
    this->re_session = regex(RE_SESSION);
    this->re_rel_session = regex(RE_REL_SESSION);
    this->re_dir = regex(RE_DIR);
    this->re_qry_speed = regex(RE_QRY_SPEED);
    this->re_qry_direction = regex(RE_QRY_DIRECTION);
    this->re_func = regex(RE_FUNC);
    setStartSessionTime();
}

tcpClient::~tcpClient()
{
    //dtor
    delete(edsession);
}

void tcpClient::setStartSessionTime(){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME,&spec);
    edsession->setCbusTime(spec);
    edsession->setEDTime(spec);
}

void tcpClient::setIp(char *ip){
    this->ip = string(ip);
}

string tcpClient::getIp(){
    return ip.c_str();
}

int tcpClient::getId(){
    return id;
}

void tcpClient::start(void *param){
    running = 1;
    logger->debug("Sending start info :%s", START_INFO);
    sendToEd(START_INFO);
    run(nullptr);
}

void tcpClient::stop(){
    running = 0;
}

void tcpClient::sendCbusMessage(int nbytes, byte b0, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7){
    char msg[CAN_MSG_SIZE];
    msg[0] = b0;
    msg[1] = b1;
    msg[2] = b2;
    msg[3] = b3;
    msg[4] = b4;
    msg[5] = b5;
    msg[6] = b6;
    msg[7] = b7;
    logger->debug("Sending message to CBUS");
    int n=nbytes;
    if (nbytes>CAN_MSG_SIZE) n = CAN_MSG_SIZE;
    can->insert_data(msg,n);
}

void tcpClient::sendCbusMessage(byte b0){
    sendCbusMessage(1,b0,0,0,0,0,0,0,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1){
    sendCbusMessage(2,b0,b1,0,0,0,0,0,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1, byte b2){
    sendCbusMessage(3,b0,b1,b2,0,0,0,0,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1, byte b2, byte b3){
    sendCbusMessage(4,b0,b1,b2,b3,0,0,0,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4){
    sendCbusMessage(5,b0,b1,b2,b3,b4,0,0,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5){
    sendCbusMessage(6,b0,b1,b2,b3,b4,b5,0,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5, byte b6){
    sendCbusMessage(7,b0,b1,b2,b3,b4,b5,b6,0);
}
void tcpClient::sendCbusMessage(byte b0,byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7){
    sendCbusMessage(8,b0,b1,b2,b3,b4,b5,b6,b7);
}

void tcpClient::canMessage(char canid,const char* msg){
    //test to send data to client tcp
    //int nbytes;
    //char buf[CAN_MSG_SIZE+1];
    try{
        char buf[30];
        memset(buf,0,20);
        sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x\n", msg[0],msg[1],msg[2],msg[3],msg[4],msg[5],msg[6],msg[7]);
        logger->debug("Tcp Client received cbus message: %s",buf);
        //memcpy(buf,msg,CAN_MSG_SIZE);
        //buf[CAN_MSG_SIZE] = '\n';
        //nbytes = write(client_sock,buf,CAN_MSG_SIZE + 1);
        //nbytes = write(client_sock,buf,nbytes);

        handleCBUS(msg);
    }
    catch(runtime_error &ex){
        logger->debug("Failed to process the can message");
        throw_line("Failed to process the can message");
    }
}

void tcpClient::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    pthread_t kalive;
    pthread_create(&kalive, nullptr, tcpClient::thread_keepalive, this);

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes<0){
            logger->debug("Error while receiving data from ED %d",nbytes);
            running = 0;
        }
        else if (nbytes>0){
            logger->debug("Received from ED:%s Bytes:%d", msg, nbytes);
            try{
                handleEDMessages(msg);
            }
            catch(const runtime_error &ex){
                logger->debug("Failed to process the can message\n%s",ex.what());
            }
        }
        if (nbytes == 0){
            logger->debug("0 bytes received. disconnecting");
            running = 0;
            break;
        }
    }
    logger->info("Quiting client connection ip:%s id:%d.",ip.c_str(),id);
    usleep(1000*1000); //1sec give some time for any pending thread to finish
    close(client_sock);
    server->removeClient(this);
}

void tcpClient::sendKeepAlive(void *param){

    long millis;
    struct timespec spec;
    struct timespec t;

    while (running){
        try{
            usleep(1000*500);//500ms
            if (running == 0){
                logger->info("Stopping keep alive process");
                break;
            }
            if (sessions.size() == 0){
                //logger->debug("Keep alive 0 sessions");
                clock_gettime(CLOCK_REALTIME,&spec);
                t = edsession->getEDTime();
                millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                if (millis > ED_KEEP_ALIVE ){
                    edsession->setEDTime(spec);
                    //send to ED
                    logger->debug("Send ED keep alive");
                    sendToEd("\n");
                }
            }
            else{
                //logger->debug("Keep alive %d sessions",sessions.size());
                std::map<int,edSession*>::iterator it = sessions.begin();
                while(it != sessions.end())
                {
                    //logger->debug("Keep alive %d loco",it->second->getLoco());
                    clock_gettime(CLOCK_REALTIME,&spec);
                    t = it->second->getEDTime();
                    millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                    if (millis > ED_KEEP_ALIVE ){
                        it->second->setEDTime(spec);
                        //send to ED
                        logger->debug("Send ED keep alive");
                        sendToEd("\n");
                    }
                    if (it->second->getLoco()>-1){
                        t = it->second->getCbusTime();
                        millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                        if (millis > CBUS_KEEP_ALIVE ){
                            it->second->setCbusTime(spec);
                            //send keep alive
                            logger->debug("Send CBUS keep alive");
                            sendCbusMessage(OPC_DKEEP,it->second->getSession());
                        }
                    }
                    else{
                        logger->debug("Loco not set keep alive");
                    }
                    it++;
                }
            }
        }//try
        catch(...){
            logger->debug("Keep alive error");
        }
    }
    logger->info("Finish keep alive process");
}

void tcpClient::handleEDMessages(char* msgptr){

    vector<string> msgs;
    string message (msgptr);
    const char *msgtemp;

    try{
        msgs = split(message,'\n', msgs);

        vector<string>::iterator it = msgs.begin();
        for (auto const& msg:msgs){
            logger->debug("Handle message:%s",msg.c_str());

            if (msg.length() == 0){
                continue;
            }
            msgtemp = msg.c_str();
            //get the name
            if (msgtemp[0] == 'N'){
                logger->debug("ED name: %s" , msgtemp);
                edsession->setEdName(msg.substr(1,msg.length()-1));
                sendToEd("\n*10\n"); //keep alive each 10 seconds
                continue;
            }

            //get hardware info
            if ((msgtemp[0] == 'H') & (msgtemp[1] == 'U')){
                edsession->setEdHW(msg.substr(2,msg.length()-2));
                logger->debug("Received Hardware info: %s" , msg.substr(2,msg.length()-2).c_str());
                sendToEd("\n*10\n"); //keep alive each 10 seconds
                //TODO wait for confirmation: expected 0xa
                continue;
            }

            if ((msgtemp[0] == '*') & (msgtemp[1] == '+')){
                logger->debug("Timer request");
                sendToEd("*10\n"); //keep alive each 10 seconds
                continue;
            }

            if ((msgtemp[0] == '*') & (msgtemp[1] == '-')){
                logger->debug("Finish timer request");
                continue;
            }

            //create session
            if (regex_match(msg,re_session)){
                logger->debug("Create session %s" , msg.c_str());
                handleCreateSession(msg);
                // wait until session is created
                logger->debug("Waiting 2 secs for session to be created.");
                usleep(2*1000000);//2 seconds
                logger->debug("Awake from 2 secs");
                if (sessions.size()>0){
                    logger->debug("Session created after 2 secs.");
                }
                else{
                    logger->debug("No session created.");
                }
                continue;
            }

            //set speed
            if (regex_match(msg,re_speed)){
                handleSpeed(msg);
                continue;
            }

            if (regex_match(msg,re_dir)){
                handleDirection(msg);
                continue;
            }

            if (regex_match(msg,re_qry_speed)){
                handleQuerySpeed(msg);
                continue;
            }

            if (regex_match(msg,re_qry_direction)){
                handleQueryDirection(msg);
                continue;
            }

            if (regex_match(msg,re_rel_session)){
                handleReleaseSession(msg);
                continue;
            }

            if (regex_match(msg,re_func)){
                handleSetFunction(msg);
                continue;
            }
        }
    }
    catch(const runtime_error &ex){
        throw_line(ex.what());
    }
    catch(...){
        throw_line("Not runtime error cought");
    }
}

std::vector<std::string> & tcpClient::split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s+' ');
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

void tcpClient::handleCBUS(const char *msg){

    unsigned char opc = msg[0];
    int loco;
    string message;
    stringstream ss;
    unsigned char speed;
    unsigned char direction ;
    unsigned char session;

    switch (opc){
    case OPC_PLOC:
        logger->debug("Checking result of request session. OPC: PLOC %02x %02x",msg[2] ,msg[3]);
        session = msg[1];
        loco = msg[2] & 0x3f;
        loco = (loco << 8) + msg[3];

        if (edsession->getLoco() != loco){
            logger->debug("PLOC %d not for this session %d. Discarding." , loco, edsession->getLoco());
            return;
        }
        edsession->setSession(session);
        //decode the DCC format
        speed = msg[4] & 0x7F; //#0111 1111
        direction = 0;
        if ((msg[4] & 0x80) > 127)   direction = 1;

        //put session in array
        edsession->setDirection(direction);
        edsession->setSpeed(speed);

        ss << "MT+";
        ss << edsession->getAddressType();
        ss << loco;
        ss << DELIM_BTLT;
        ss << edsession->getLocoName();
        ss << '\n';

        message = ss.str();
        logger->debug("Ack client session created %d for loco %d :%s" ,session, loco, message.c_str());
        sendToEd(message);

        logger->debug("Adding loco %d to sessions",  loco);
        logger->info("Loco %d acquired",loco);
        sessions.insert(pair<int,edSession*>(loco,edsession));
        edsession = new edSession(logger);
        setStartSessionTime();

        //set speed mode 128 to can
        logger->debug("Sending speed mode 128 to CBUS");
        sendCbusMessage(OPC_STMOD, session, 0);

        //send the labels to client
        ss.clear();ss.str();
        ss << "MTLS";
        ss << loco;
        ss << EMPTY_LABELS;
        ss << "S";
        ss << generateFunctionsLabel(loco);
        ss << '\n';

        logger->debug("Sending labels");

        sendToEd(ss.str());

        logger->debug("Sending speed mode 128 to ED");
        ss.clear();ss.str();
        ss << "MT";
        ss << edsession->getAddressType();
        ss << loco;
        ss << DELIM_BTLT;
        ss << "s0\n";

        sendToEd(ss.str());
        break;
    case OPC_ERR:
        logger->debug("CBUS Error message");
        loco = msg[2] & 0x3f;
        loco = (loco << 8) + msg[3];

        if (loco != edsession->getLoco()){
            logger->debug("Error message for another client. Different loco number %d %d. Discarding.",edsession->getLoco(), loco);
            return;
        }

        switch (msg[3]){
        case 1:
            logger->info("Can not create session. Reason: stack full");
            break;
        case 2:
            logger->info("Err: Loco %d TAKEN", loco);
            break;
        case 3:
            logger->info("Err: No session %d" , loco);
            break;
        default:
            logger->info("Err code: %d" , msg[3]);
        }
        break;
    }
}

string tcpClient::generateFunctionsLabel(int loco){
    stringstream ss;
    ss << "MTA";
    ss << loco;
    ss << DELIM_BTLT;
    string s = ss.str();
    for (int f=0;f<FN_SIZE;f++){
        ss << s;
        ss << "F0";
        ss << f;
        ss << "\n";
    }
    ss << s;
    ss << "V0\n";
    ss << s;
    ss << "R1\n";
    ss << s;
    ss << "s0\n";

    return ss.str();
}

void tcpClient::sendToEd(string msg){
    unsigned int nbytes;
    logger->debug("Send to ED:%s",msg.c_str());
    nbytes = write(client_sock,msg.c_str(),msg.length());
    if (nbytes != msg.length()){
        logger->error("Fail to send message %s to ED", msg.c_str());
    }
}

void tcpClient::handleCreateSession(string message){
    const char *msg = message.c_str();
    logger->debug("Handle create session: %s", msg);
    unsigned char Hb,Lb;
    int loco,i;
    //create session
    if ( (msg[3] == 'S') | (msg[3] == 's') | (msg[3] == 'L') | (msg[3] == 'l') ){

        logger->debug("Address type %c",msg[3]);
        edsession->setAddressType(msg[3]);
        i = message.find(";>");
        if (i > 0){
            edsession->setLocoName(message.substr(i+2,(message.length()-i-2)));
        }
        //get loco
        loco = getLoco(message);
        edsession->setLoco(loco);

        //send the can data
        logger->info("Request session for loco %d",loco);
        logger->debug("Put CAN session request in the queue for loco %d" , loco);

        Hb = 0;
        if ((loco > 127) | (msg[3] == 'L') | (msg[3] == 'l')){
            Hb = loco >> 8 | 0xC0;
            Lb = loco & 0xFF;
        }
        else Lb = loco & 0xFF;

        sendCbusMessage(OPC_RLOC,Hb,Lb);
    }
}

void tcpClient::handleReleaseSession(string message){

    logger->debug("Handle release session %s", message.c_str());
    //release session
    int i = message.find("*");

    //all sessions
    if (i>0){
        logger->debug("Releasing all %d sessions",sessions.size());
        std::map<int,edSession*>::iterator it = this->sessions.begin();
        while(it != this->sessions.end())
        {
            logger->info("Releasing session for loco %d" , it->second->getLoco());
            //usleep(50000);//50ms
            sendCbusMessage(OPC_KLOC,it->second->getSession());
            it++;
        }
        //clear sessions
        usleep(1000*1000);//1s
        it = this->sessions.begin();
        while(it != this->sessions.end())
        {
            delete(it->second);
            it++;
        }
        this->sessions.clear();
        //inform the ED
        sendToEd("\n");
        return;
    }

    //one session
    int loco = getLoco(message);
    logger->debug("Releasing session for loco KLOC %d" , loco);

    char sesid = this->sessions[loco]->getSession();
    //send the can data
    sendCbusMessage(OPC_KLOC,sesid);
    usleep(1000*1000);//1s
    delete(this->sessions[loco]);
    this->sessions.erase(loco);
    //inform the ED
    sendToEd("\n");
}


void tcpClient::handleDirection(string message){
    //const char* msg = message.c_str();

    logger->debug("Handle Direction request %s", message.c_str());
    sendToEd(message + "\n");

    //get the direction
    int i = message.find(">R");
    logger->debug("Extracted direction: %s" ,  message.substr(i+2,1).c_str());
    byte d = atoi(message.substr(i+2,1).c_str());

    i = message.find("*");
    //all sessions
    if (i > 0){
        logger->debug("Set direction for all sessions");
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            it->second->setDirection(d);
            logger->debug("Set direction %d for loco %d" , d ,it->second->getLoco());
            sendCbusMessage(OPC_DSPD,it->second->getSession(),d*BS+it->second->getSpeed());
            it++;
        }
        return;
    }

    //one session
    int loco = getLoco(message);
    int sesid = sessions[loco]->getSession();
    sessions[loco]->setDirection(d);
    logger->debug("Set direction %d for loco %d" , d ,loco);
    sendCbusMessage(OPC_DSPD,sesid, d * BS + sessions[loco]->getSpeed());

}

void tcpClient::handleSpeed(string message){

    string speedString;

    logger->debug("Handle speed request %s", message.c_str());

    int i = message.find(">V");
    if (i > 0){
        int s = i + 2;
        logger->debug("Extracted speed: %s", message.substr(s,message.length()-s).c_str());
        speedString = message.substr(s,message.length()-s);
    }
    else{
        i = message.find(">X");
        if (i > 0) speedString = "X";
        else{
            logger->debug("Bad speed message format. Discarding.");
            return;
        }
    }
    int speed = 0;

    if (speedString == "X") speed = 1;
    else{
        speed = atoi(speedString.c_str());
        if (speed != 0) speed++;
    }

    stringstream ss;

    i = message.find("*");
    //all sessions
    if (i > 0){
        logger->debug("Set speed %d for all sessions",speed);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            it->second->setSpeed(speed);
            logger->debug("Set speed %d for loco %d" ,speed, it->second->getLoco());
            //logger->debug("Speed dir byte %02x",it->second->getDirection() * BS + it->second->getSpeed() );
            sendCbusMessage(OPC_DSPD, it->second->getSession(), it->second->getDirection() * BS + speed);
            it++;
        }

        ss.clear();ss.str("MTA*<;>V");
        //ss << it->second->getAddressType();
        //ss << it->second->getLoco();
        //ss << DELIM_BTLT;
        //ss << "V";
        if (speed == 1) ss << "0";
        else ss << speed;
        ss << "\n";
        sendToEd(message);

        return;
    }

    //one session
    int loco = getLoco(message);
    edSession* session = sessions[loco];
    logger->debug("Set speed %d for loco %d" ,speed, loco);
    session->setSpeed(speed);
    sendCbusMessage(OPC_DSPD, session->getSession(), session->getDirection() * BS + speed);

    ss.clear();ss.str();
    ss << "MTA";
    ss << session->getAddressType();
    ss << session->getLoco();
    ss << DELIM_BTLT;
    ss << "V";
    if (session->getSpeed() == 1) ss << "0";
    else ss << (int)session->getSpeed();
    ss << "\n";

    sendToEd(ss.str());
}


void tcpClient::handleQueryDirection(string message){

    logger->debug("Query Direction found %s", message.c_str());
    int i = message.find("*");
    stringstream ss;
    if (i > 0){
        logger->debug("Query direction for all locos");
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            ss << "MTA";
            ss << it->second->getAddressType();
            ss << it->second->getLoco();
            ss << DELIM_BTLT;
            ss << "R";
            ss << (int)it->second->getDirection();
            ss << "\n";
            it++;
        }
        sendToEd(ss.str());
        return;
    }

    //specific loco
    int loco = getLoco(message);
    edSession *session = sessions[loco];
    ss.clear();ss.str();
    ss << "MTA";
    ss << session->getAddressType();
    ss << session->getLoco();
    ss << DELIM_BTLT;
    ss << "R";
    ss << (int)session->getDirection();
    ss << "\n";
    sendToEd(ss.str());
}

void tcpClient::handleQuerySpeed(string message){

    logger->debug("Query speed found %s",message.c_str());
    int i = message.find("*");
    stringstream ss;
    if (i > 0){
        logger->debug("Query speed for all locos");
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            ss << "MTA";
            ss << it->second->getAddressType();
            ss << it->second->getLoco();
            ss << DELIM_BTLT;
            ss << "V";
            if (it->second->getSpeed() == 1) ss << "0";
            else ss << (int)it->second->getSpeed();
            ss << "\n";
            it++;
        }
        sendToEd(ss.str());
        return;
    }

    //specific loco
    int loco = getLoco(message);
    edSession *session = sessions[loco];
    ss.clear();ss.str();
    ss << "MTA";
    ss << session->getAddressType();
    ss << session->getLoco();
    ss << DELIM_BTLT;
    ss << "V";
    if (session->getSpeed() == 1) ss << "0";
    else ss << (int)session->getSpeed();
    ss << "\n";
    sendToEd(ss.str());
}

void tcpClient::handleSetFunction(string message){

    logger->debug("Set function request found %s",message.c_str());

    // the ED always sends an on and off we will consider just the on and toggle the function internally
    //get the function

    int i = message.find(">F");
    logger->debug("Extracted on/off: %s func: %s" ,message.substr(i+2,1).c_str(), message.substr(i+3,message.length()-(i+3)).c_str());
    int onoff = atoi(message.substr(i+2,1).c_str());
    int fn = atoi(message.substr(i+3,message.length()-(i+3)).c_str());
    i = message.find("*");
    edSession *session;

    //all sessions
    if ( i > 0){
        logger->debug("Setting function for all sessions");
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            session = it->second;
            if ((session->getFnType(fn) == 1) & (onoff == 0) ){
                logger->debug("Fn Message for toggle fn and for a off action. Discarding");
            }
            else{
                logger->debug("Previous FN state %02x" , session->getFnState(fn));
                sendFnMessages(session,fn,message);
                logger->debug("Last FN state %02x" , session->getFnState(fn));
            }
            it++;
        }
        return;
    }
    //one session
    int loco = getLoco(message);
    session = sessions[loco];

    if ((session->getFnType(fn) == 1) & (onoff == 0) ){
        logger->debug("Fn Message for toggle fn and for a off action. Discarding");
    }
    else{
        sendFnMessages(session,fn,message);
    }
}

void tcpClient::sendFnMessages(edSession* session, int fn, string message){

    logger->debug("Set function %d for loco %d" , fn, session->getLoco());

    byte fnbyte = 1;
    byte fnbyte2 = 0;
    stringstream ss;

    //1 is F0(FL) to F4
    //2 is F5 to F8
    //3 is F9 to F12
    //4 is F13 to F19
    //5 is F20 to F28
    if ((4 < fn) & (fn < 9))    fnbyte = 2;
    if ((8 < fn) & (fn < 13))   fnbyte = 3;
    if ((12 < fn) & (fn < 20))  fnbyte = 4;
    if ((19 < fn) & (fn < 29))  fnbyte = 5;

    if (session->getFnState(fn) == 1) session->setFnState(fn,0);
    else session->setFnState(fn,1);

    //send status to ED
    int i = message.find(">F");
    logger->debug("message: %s", message.substr(0, i+2).c_str());
    //logging.debug(str(session.getFnState(fn)))

    ss << message.substr(0, i+2);
    ss << (int)session->getFnState(fn);
    ss << fn;
    ss << "\n";

    sendToEd(ss.str());

    //send msg to CBUS
    fnbyte2 = session->getDccByte(fn);
    sendCbusMessage(OPC_DFUN,session->getSession(),fnbyte,fnbyte2);
}

int tcpClient::getLoco(string msg){
    //expected format
    //MT+S45<;>Name
    try{
        int i = msg.find_first_of("<");
        logger->debug("Extracted loco: %s", msg.substr(4,i-4).c_str());
        int loco = atoi(msg.substr(4,i-4).c_str());
        return loco;
    }
    catch(...){
        return 0;
    }
}
