#include "tcpClient.h"
#include <stdio.h>

tcpClient::tcpClient(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr,int id, nodeConfigurator *config)
{
    //ctor

    this->server = server;
    this->can = can;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    this->config = config;
    logger->debug("Client %d created", id);
    edsession = new edSession(logger);
    edsession->setNodeConfigurator(config);
    edsession->getMomentaryFNs();
    this->re_speed = regex(RE_SPEED);
    this->re_session = regex(RE_SESSION);
    this->re_rel_session = regex(RE_REL_SESSION);
    this->re_dir = regex(RE_DIR);
    this->re_qry_speed = regex(RE_QRY_SPEED);
    this->re_qry_direction = regex(RE_QRY_DIRECTION);
    this->re_func = regex(RE_FUNC);
    this->re_turnout = regex(RE_TURNOUT);
    this->re_turnout_generic = regex(RE_TURNOUT_GENERIC);
    this->re_idle = regex(RE_IDLE);
    setStartSessionTime();
    this->clientType = ClientType::ED;

    pthread_mutex_init(&m_mutex_in_cli, NULL);
    pthread_cond_init(&m_condv_in_cli, NULL);
}

tcpClient::~tcpClient()
{
    //dtor
    this->sessions.clear();
    delete(edsession);
    pthread_mutex_destroy(&m_mutex_in_cli);
    pthread_cond_destroy(&m_condv_in_cli);
}

void tcpClient::setTurnout(Turnout* turnouts){
    this->turnouts = turnouts;
}

void tcpClient::setStartSessionTime(){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME,&spec);
    edsession->setCbusTime(spec);
    edsession->setEDTime(spec);
}

void tcpClient::start(void *param){
    running = 1;
    stringstream ss;
    ss << SOFT_VERSION;ss << "\n";
    sendToEd(ss.str());
    ss << "\n";
    ss.clear();ss.str("");
    ss << START_INFO_RL;ss << "\n";
    ss << START_INFO_PPA;ss << "\n";
    ss << turnouts->getStartInfo();ss << "\n";
    ss << START_INFO_PRT;ss << "\n";
    ss << START_INFO_PRL;ss << "\n";
    //ss << "PRL]\[MAINT}|{Main}|{]\[YARD1}|{Yard1}|{";ss << "\n\n";
    ss << START_INFO_RCC;ss << "\n";
    ss << START_INFO_PW;ss << "\n";
    //logger->debug("[%d] [tcpClient] Sending start info :%s",id, ss.str().c_str());
    sendToEd(ss.str());
    run(nullptr);
}

void tcpClient::stop(){
    running = 0;
    usleep(1000*1000);
    close(client_sock);
}

void tcpClient::canMessage(int canid,const char* msg, int dlc){
    struct can_frame frame;

    frame.can_id = canid;
    frame.can_dlc = dlc;
    frame.data[0] = msg[0];
    frame.data[1] = msg[1];
    frame.data[2] = msg[2];
    frame.data[3] = msg[3];
    frame.data[4] = msg[4];
    frame.data[5] = msg[5];
    frame.data[6] = msg[6];
    frame.data[7] = msg[7];

    //pthread_mutex_lock(&m_mutex_in_cli);
    //cout << "#######@@@###@#@###@" << endl;
    in_msgs.push(frame);
    //pthread_mutex_unlock(&m_mutex_in_cli);
    //pthread_cond_signal(&m_condv_in_cli);
}

void tcpClient::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    pthread_t kalive;
    pthread_create(&kalive, nullptr, tcpClient::thread_keepalive, this);
    pthread_t cbusin;
    pthread_create(&cbusin, nullptr, tcpClient::thread_processcbus, this);

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes<0){
            logger->debug("[%d] [tcpClient] Error while receiving data from ED %d",id,nbytes);
            running = 0;
        }
        else if (nbytes>0){
            logger->debug("[%d] [tcpClient] Received from ED:%s Bytes:%d",id, msg, nbytes);
            try{
                handleEDMessages(msg);
            }
            catch(const runtime_error &ex){
                logger->debug("[%d][tcpClient] Failed to process the can message\n%s",id,ex.what());
            }
        }
        if (nbytes == 0){
            logger->debug("[%d] [tcpClient] 0 bytes received. disconnecting",id);
            running = 0;
            break;
        }
    }
    logger->info("[%d] [tcpClient] Quiting client connection ip:%s id:%d.",id, ip.c_str(),id);

    usleep(2000*1000); //1sec give some time for any pending thread to finish
    try{
        pthread_cancel(kalive);
        pthread_cancel(cbusin);
        server->removeClient(this);
    }
    catch(...){
        logger->error("[tcpClient] Failed to stop the tcp client.");
    }

}

void tcpClient::sendKeepAlive(void *param){

    long millis;
    struct timespec spec;
    struct timespec t;

    while (running){
        try{
            usleep(1000*500);//500ms
            if (running == 0){
                logger->info("[%d] [tcpClient] Stopping keep alive process",id);
                break;
            }
            if (sessions.size() == 0){
                //logger->debug("[%d] Keep alive 0 sessions",id);
                clock_gettime(CLOCK_REALTIME,&spec);
                t = edsession->getEDTime();
                millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                if (millis > ED_KEEP_ALIVE ){
                    //send to ED
                    logger->debug("[%d] [tcpClient] Send ED keep alive",id);
                    sendToEd("*10\n");
                    edsession->setEDTime(spec);
                }
            }
            else{
                //logger->debug("[%d] Keep alive %d sessions",id,sessions.size());
                std::map<int,edSession*>::iterator it = sessions.begin();
                while(it != sessions.end())
                {
                    //logger->debug("[%d] Keep alive %d loco",id,it->second->getLoco());
                    clock_gettime(CLOCK_REALTIME,&spec);
                    t = it->second->getEDTime();
                    millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                    if (millis > ED_KEEP_ALIVE ){
                        it->second->setEDTime(spec);
                        //send to ED
                        logger->debug("[%d] [tcpClient] Send ED keep alive",id);
                        sendToEd("*10\n");
                    }
                    if (it->second->getLoco()>-1){
                        t = it->second->getCbusTime();
                        millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                        if (millis > CBUS_KEEP_ALIVE ){
                            it->second->setCbusTime(spec);
                            //send keep alive
                            logger->debug("[%d] [tcpClient] Send CBUS keep alive",id);
                            sendCbusMessage(OPC_DKEEP,it->second->getSession());
                        }
                    }
                    else{
                        logger->debug("[%d] [tcpClient] Loco not set keep alive",id);
                    }
                    it++;
                }
            }
        }//try
        catch(...){
            logger->debug("[%d] [tcpClient] Keep alive error",id);
        }
    }
    logger->info("[%d] [tcpClient] Finish keep alive process",id);
}

void tcpClient::handleEDMessages(char* msgptr){

    vector<string> msgs;
    string message (msgptr);
    const char *msgtemp;

    try{
        msgs = split(message,'\n', msgs);

        //vector<string>::iterator it = msgs.begin();
        for (auto const& msg:msgs){
            logger->debug("[%d] [tcpClient] Handle message:%s",id,msg.c_str());

            if (msg.length() == 0){
                continue;
            }
            msgtemp = msg.c_str();
            //get the name
            if (msgtemp[0] == 'N'){
                logger->debug("[%d] [tcpClient] ED name: %s" ,id, msgtemp);
                edsession->setEdName(msg.substr(1,msg.length()-1));
                sendToEd("*10\n"); //keep alive each 10 seconds
                continue;
            }

            //get hardware info
            if ((msgtemp[0] == 'H') && (msgtemp[1] == 'U')){
                edsession->setEdHW(msg.substr(2,msg.length()-2));
                logger->debug("[%d] [tcpClient] Received Hardware info: %s" , id,msg.substr(2,msg.length()-2).c_str());
                sendToEd("\n*10\n"); //keep alive each 10 seconds
                //TODO wait for confirmation: expected 0xa
                continue;
            }

            if ((msgtemp[0] == '*') && (msgtemp[1] == '+')){
                logger->debug("[%d] [tcpClient] Timer request",id);
                sendToEd("*10\n"); //keep alive each 10 seconds
                continue;
            }

            if ((msgtemp[0] == '*') && (msgtemp[1] == '-')){
                logger->debug("[%d] [tcpClient] Finish timer request",id);
                continue;
            }

            //create session
            if (regex_match(msg,re_session)){
                logger->debug("[%d] [tcpClient] Create session %s" ,id, msg.c_str());
                int loco = handleCreateSession(msg);
                // wait until session is created
                logger->debug("[%d] [tcpClient] Waiting for session to be created.",id);
                int loop = 0;
                while (loop < 4 && loco > 0){
                    usleep(500*1000);
                    if (sessions.size()>0){
                        if (sessions.find(loco) != sessions.end()){
                            logger->debug("[%d] [tcpClient] Session created for loco %d.", id, loco);
                            loop = 10;
                        }
                    }
                    loop++;
                }
                if (sessions.find(loco) == sessions.end()){
                    logger->debug("[%d] [tcpClient] No session created for loco %d.",id, loco);
                }
                continue;
            }

            //set speed
            if (regex_match(msg,re_speed)){
                handleSpeed(msg);
                continue;
            }
            //idle. basically sets the speed to 0
            if (regex_match(msg,re_idle)){
                handleIdle(msg);
                continue;
            }
            //set direction
            if (regex_match(msg,re_dir)){
                handleDirection(msg);
                continue;
            }
            //query speed
            if (regex_match(msg,re_qry_speed)){
                handleQuerySpeed(msg);
                continue;
            }
            //query direction
            if (regex_match(msg,re_qry_direction)){
                handleQueryDirection(msg);
                continue;
            }
            //release session
            if (regex_match(msg,re_rel_session)){
                handleReleaseSession(msg);
                continue;
            }
            //set unset FNs
            if (regex_match(msg,re_func)){
                handleSetFunction(msg);
                continue;
            }
            //turnouts
            if (regex_match(msg,re_turnout)){
                handleTurnout(msg);
                continue;
            }
            if (regex_match(msg,re_turnout_generic)){
                handleTurnoutGeneric(msg);
                continue;
            }

            logger->debug("[%d] [tcpClient] Support not implemented %s",id,msg.c_str());
        }
    }
    catch(const runtime_error &ex){
        //throw_line(ex.what());
        logger->debug("[tcpClient] Not runtime error cought. %s",ex.what() );
        cout << "[tcpClient] Not runtime error cought" << ex.what() << endl;
    }
    catch(...){
        logger->debug("[tcpClient] Not runtime error cought");
        cout << "[tcpClient] Not runtime error cought" << endl;
    }
}

void tcpClient::processCbusQueue(void *param){
    struct can_frame frame;
    char buf[30];
    logger->debug("[%d] [tcpClient] Tcp Client cbus thread read cbus queue",id);
    while (running){
        if (!in_msgs.empty()){
            //pthread_mutex_lock(&m_mutex_in_cli);
            frame = in_msgs.front();
            in_msgs.pop();
            //pthread_mutex_unlock(&m_mutex_in_cli);
            //pthread_cond_signal(&m_condv_in_cli);
            try{
                memset(buf,0,sizeof(buf));
                sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x\n", frame.data[0],frame.data[1],frame.data[2],frame.data[3],frame.data[4],frame.data[5],frame.data[6],frame.data[7]);
                logger->debug("[%d] [tcpClient] Tcp Client received cbus message: %s",id,buf);
                //cout << "#######@@@###@#@###@" << endl;
                handleCBUS(frame.data);
            }
            catch(runtime_error &ex){
                logger->debug("[%d] [tcpClient] Failed to process the can message",id);
                //throw_line("Failed to process the can message");
            }
            catch(...){
                logger->debug("[%d] [tcpClient] Failed to process the can message",id);
            }
        }
        usleep(4000);
    }
}

void tcpClient::handleCBUS(unsigned char *msg){

    unsigned char opc = msg[0];
    int loco,tcode;
    string message;
    stringstream ss;
    unsigned char speed;
    unsigned char direction ;
    unsigned char session;
    char stype;

    logger->info("[%d] [tcpClient] Processing CBUS message %02x",id,opc);
    switch (opc){
        case OPC_PLOC:
            logger->debug("[%d] [tcpClient] Checking result of request session. OPC: PLOC %02x %02x",id,msg[2] ,msg[3]);
            session = msg[1];
            loco = msg[2] & 0x3f;
            loco = (loco << 8) + msg[3];

            if (edsession->getLoco() != loco){
                logger->debug("[%d] [tcpClient] PLOC %d not for this session %d. Discarding." , id,loco, edsession->getLoco());
                return;
            }
            logger->info("[%d] [tcpClient] Loco %d acquired",id,loco);
            edsession->setSession(session);
            //decode the DCC format
            speed = msg[4] & 0x7F; //#0111 1111
            direction = 0;
            if ((msg[4] & 0x80) > 127)   direction = 1;

            //put session in array
            edsession->setDirection(direction);
            edsession->setSpeed(speed);

            logger->debug("[%d] [tcpClient] Ack client session created %d for loco %d :%s" ,id,session, loco, message.c_str());
            logger->debug("[%d] [tcpClient] Adding loco %d to sessions", id, loco);
            sessions.insert(pair<int,edSession*>(loco,edsession));

            stype = edsession->getCharSessionType();

            ss << "M"; ss << stype; ss << "+";
            ss << edsession->getAddressType();
            ss << loco;
            ss << DELIM_BTLT;
            ss << edsession->getLocoName();
            ss << '\n';

            message = ss.str();

            sendToEd(message);

            //set speed mode 128 to can
            logger->debug("[%d] [tcpClient] Sending speed mode 128 to CBUS",id);
            sendCbusMessage(OPC_STMOD, session, 0);

            //send the labels to client
            ss.clear();ss.str();
            ss << "M"; ss << stype ; ss <<"L" ; ss << edsession->getAddressType();
            ss << loco;
            ss << EMPTY_LABELS;
            //ss << "S";
            ss << generateFunctionsLabel(loco, stype, edsession->getAddressType());
            ss << '\n';

            logger->debug("[%d] [tcpClient] Sending labels",id);

            sendToEd(ss.str());

            logger->debug("[%d] [tcpClient] Sending speed mode 128 to ED",id);
            ss.clear();ss.str();
            ss << "M";
            ss << stype;
            ss << edsession->getAddressType();
            ss << loco;
            ss << DELIM_BTLT;
            ss << "s0\n";
            sendToEd(ss.str());
            //create new edsession object
            edsession = new edSession(logger);
            setStartSessionTime();
        break;

        case OPC_ERR:
            logger->debug("[%d] [tcpClient] CBUS Error message",id);
            loco = msg[2] & 0x3f;
            loco = (loco << 8) + msg[3];

            if (loco != edsession->getLoco()){
                logger->debug("[%d] [tcpClient] Error message for another client. Different loco number %d %d. Discarding.",id,edsession->getLoco(), loco);
                return;
            }

            switch (msg[3]){
            case 1:
                logger->info("[%d] [tcpClient] Can not create session. Reason: stack full",id);
                break;
            case 2:
                logger->info("[%d] [tcpClient] Err: Loco %d TAKEN",id, loco);
                break;
            case 3:
                logger->info("[%d] [tcpClient] Err: No session %d" ,id, loco);
                break;
            default:
                logger->info("[%d] [tcpClient] Err code: %d" ,id, msg[3]);
            }
        break;
        case OPC_ASOF:
            tcode = msg[3];
            tcode = (tcode << 8) | msg[4];
            logger->info("[%d] [tcpClient] ASOF received. Checking turnout: %d." ,id,tcode);
            // check the turnout state and inform the ED
            if (turnouts->exists(tcode)){
                logger->debug("[%d] [tcpClient] Found turnout: %d. Closing" ,id,tcode);
                turnouts->CloseTurnout(tcode);
                sendToEd(turnouts->getTurnoutMsg(tcode) + "\n");
            }
        break;

        case OPC_ASON:
            tcode = msg[3];
            tcode = (tcode << 8) | msg[4];
            logger->info("[%d] [tcpClient] ASON received. Checking turnout: %d." ,id,tcode);
            // check the turnout state and inform the ED
            if (turnouts->exists(tcode)){
                logger->debug("[%d] [tcpClient] Found turnout: %d. Throwing" ,id,tcode);
                turnouts->ThrownTurnout(tcode);
                sendToEd(turnouts->getTurnoutMsg(tcode) + "\n");
            }
        break;
    }
}

string tcpClient::generateFunctionsLabel(int loco,char stype, char adtype){
    stringstream ss;
    ss << "M" ;
    ss << stype; // session type T or S
    ss << "A";
    ss << adtype; // S or L
    ss << loco;
    ss << DELIM_BTLT;
    string s = ss.str();
    ss.clear();ss.str("");
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
    logger->debug("[%d] [tcpClient] Send to ED:%s",id, msg.c_str());
    nbytes = write(client_sock,msg.c_str(),msg.length());
    if (nbytes != msg.length()){
        logger->error("[tcpClient] Fail to send message %s to ED",id, msg.c_str());
    }
}
//return the loco
int tcpClient::handleCreateSession(string message){
    const char *msg = message.c_str();
    logger->debug("[%d] [tcpClient] Handle create session: %s",id, msg);
    unsigned char Hb,Lb;
    int loco,i;
    //create session
    if ( (msg[3] == 'S') | (msg[3] == 's') | (msg[3] == 'L') | (msg[3] == 'l') ){

        switch (msg[1]){
            case 'T':
                edsession->setSessionType(SessionType::MULTI_THROTTLE);
            break;
            case 'S':
                edsession->setSessionType(SessionType::MULTI_SESSION);
            break;
            default:
               edsession->setSessionType(SessionType::MULTI_THROTTLE);
        }

        logger->debug("[%d] [tcpClient] Address type %c",id,msg[3]);
        edsession->setAddressType(msg[3]);
        i = message.find(";>");
        if (i > 0){
            edsession->setLocoName(message.substr(i+2,(message.length()-i-2)));
        }
        //get loco
        loco = getLoco(message);
        edsession->setLoco(loco);

        //send the can data
        logger->info("[%d] [tcpClient] Request session for loco %d",id,loco);
        logger->debug("[%d] [tcpClient] Put CAN session request in the queue for loco %d" ,id, loco);

        Hb = 0;
        if ((loco > 127) | (msg[3] == 'L') | (msg[3] == 'l')){
            Hb = loco >> 8 | 0xC0;
            Lb = loco & 0xFF;
        }
        else Lb = loco & 0xFF;

        sendCbusMessage(OPC_RLOC,Hb,Lb);
        return loco;
    }
    return -1;
}

void tcpClient::handleReleaseSession(string message){

    logger->debug("[%d] [tcpClient] Handle release session %s",id, message.c_str());
    //release session
    int i = message.find("*");
    byte sesid;
    char stype = message.c_str()[1];
    //all sessions
    if (i>0){
        logger->debug("[%d] [tcpClient] Releasing all %c sessions",id,stype);
        std::map<int,edSession*>::iterator it = this->sessions.begin();
        while(it != this->sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                logger->info("[%d] [tcpClient] Releasing session for loco %d" ,id, it->second->getLoco());
                sesid = it->second->getSession();
                sendCbusMessage(OPC_KLOC, sesid);
            }
            it++;
        }
        //clear sessions

        usleep(1000*500);//300ms
        it = this->sessions.begin();
        logger->info("[%d] [tcpClient] Dealocating sessions" ,id);
        while(it != this->sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                logger->info("[%d] [tcpClient] Dealocating loco %d" ,id, it->second->getLoco());
                delete(it->second);
                sessions.erase(it);
            }
            it++;
        }
        //inform the ED
        logger->info("[%d] [tcpClient] Finished dealocatting sessions" ,id);
        sendToEd("\n");
        return;
    }

    //one session
    int loco = getLoco(message);
    logger->debug("[%d] [tcpClient] Releasing session for loco KLOC %d" ,id, loco);
    //check if exists
    if (sessions.find(loco) != sessions.end()){
        sesid = this->sessions[loco]->getSession();
        //send the can data
        sendCbusMessage(OPC_KLOC,sesid);
        usleep(1000*200);//300ms
        delete(this->sessions[loco]);
        this->sessions.erase(loco);
    }
    else logger->debug("[%d] [tcpClient] Loco %d not allocated" ,id,loco);
    //inform the ED
    sendToEd("\n");
}


void tcpClient::handleDirection(string message){
    //const char* msg = message.c_str();

    logger->debug("[%d] [tcpClient] Handle Direction request %s",id, message.c_str());
    sendToEd(message + "\n");

    //get the direction
    int i = message.find(">R");
    logger->debug("[%d] [tcpClient] Extracted direction: %s" , id, message.substr(i+2,1).c_str());
    byte d = atoi(message.substr(i+2,1).c_str());

    i = message.find("*");
    char stype = message.c_str()[1];
    //all sessions
    if (i > 0){
        logger->debug("[%d] [tcpClient] Set direction for all sessions",id);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                it->second->setDirection(d);
                logger->debug("[%d] [tcpClient] Set direction %d for loco %d" ,id, d ,it->second->getLoco());
                sendCbusMessage(OPC_DSPD,it->second->getSession(),d*BS+it->second->getSpeed());
            }
            it++;
        }
        return;
    }

    //one session
    int loco = getLoco(message);
    if (sessions.find(loco) != sessions.end()){
        int sesid = sessions[loco]->getSession();
        sessions[loco]->setDirection(d);
        logger->debug("[%d] [tcpClient] Set direction %d for loco %d" ,id, d ,loco);
        int speed = sessions[loco]->getSpeed();
        if (speed == 1) speed++;
        sendCbusMessage(OPC_DSPD,sesid, d * BS + speed);
    }
    else logger->debug("[%d] [tcpClient] Loco %d not allocated" ,id,loco);
}

void tcpClient::handleSpeed(string message){

    string speedString;

    logger->debug("[%d] [tcpClient] Handle speed request %s",id, message.c_str());

    int i = message.find(">V");
    if (i > 0){
        int s = i + 2;
        logger->debug("[%d] [tcpClient] Extracted speed: %s",id, message.substr(s,message.length()-s).c_str());
        speedString = message.substr(s,message.length()-s);
    }
    else{
        i = message.find(">X");
        if (i > 0) speedString = "X";
        else{
            logger->debug("[%d] [tcpClient] Bad speed message format. Discarding.",id);
            return;
        }
    }
    int speed = 0;
    int edspeed = 0;

    if (speedString == "X"){//emergency stop
        speed = 1;
        edspeed = 0;
    }
    else{
        speed = atoi(speedString.c_str());
        edspeed = speed;
        if (speed == 1){//can't send speed 1 (emergency stop) to cancmd
            speed++;
        }
    }

    stringstream ss;

    i = message.find("*");
    char stype = message.c_str()[1];
    //all sessions
    if (i > 0){
        logger->debug("[%d] [tcpClient] Set speed %d for all sessions",id,edspeed);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                it->second->setSpeed(edspeed);
                logger->debug("[%d] Set speed %d for loco %d" ,id,edspeed, it->second->getLoco());
                sendCbusMessage(OPC_DSPD, it->second->getSession(), it->second->getDirection() * BS + speed);
                usleep(1000*10);//wait 10ms
            }
            it++;
        }
        sendToEd(message);

        return;
    }

    //one session
    int loco = getLoco(message);
    if (sessions.find(loco) != sessions.end()){
        edSession* session = sessions[loco];
        logger->debug("[%d] [tcpClient] Set speed %d for loco %d" ,id,edspeed, loco);
        session->setSpeed(edspeed);
        sendCbusMessage(OPC_DSPD, session->getSession(), session->getDirection() * BS + speed);

        ss.clear();ss.str();
        ss << "M";
        ss << session->getCharSessionType();
        ss << "A";
        ss << session->getAddressType();
        ss << session->getLoco();
        ss << DELIM_BTLT;
        ss << "V";
        //if (session->getSpeed() == 1) ss << "0";
        //else ss << (int)session->getSpeed();
        ss << edspeed;
        ss << "\n";

        sendToEd(ss.str());
        usleep(1000*10);//wait 10ms
    }
    else logger->debug("[%d] [tcpClient] Loco %d not allocated" ,id,loco);
}

void tcpClient::handleIdle(string message){

    string speedString;
    int edspeed = 0;
    stringstream ss;

    logger->debug("[%d] [tcpClient] Handle idle request %s",id, message.c_str());
    int i = message.find("*");
    char stype = message.c_str()[1];
    //all sessions
    if (i > 0){
        logger->debug("[%d] [tcpClient] Set speed %d for all sessions",id,edspeed);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                it->second->setSpeed(edspeed);
                logger->debug("[%d] Set speed %d for loco %d" ,id,edspeed, it->second->getLoco());
                sendCbusMessage(OPC_DSPD, it->second->getSession(), it->second->getDirection() * BS + edspeed);
                usleep(1000*10);//wait 10ms
            }
            it++;
        }
        sendToEd(message);
        return;
    }

    //one session
    int loco = getLoco(message);
    if (sessions.find(loco) != sessions.end()){
        edSession* session = sessions[loco];
        logger->debug("[%d] [tcpClient] Set speed %d for loco %d" ,id,edspeed, loco);
        session->setSpeed(edspeed);
        sendCbusMessage(OPC_DSPD, session->getSession(), session->getDirection() * BS + edspeed);
        ss.clear();ss.str();
        ss << "M";
        ss << session->getCharSessionType();
        ss << "A";
        ss << session->getAddressType();
        ss << session->getLoco();
        ss << DELIM_BTLT;
        ss << "V";
        //if (session->getSpeed() == 1) ss << "0";
        //else ss << (int)session->getSpeed();
        ss << edspeed;
        ss << "\n";

        sendToEd(ss.str());
        usleep(1000*10);//wait 10ms
    }
    else logger->debug("[%d] [tcpClient] Loco %d not allocated" ,id,loco);

    sendToEd(message);
}

void tcpClient::handleQueryDirection(string message){

    logger->debug("[%d] [tcpClient] Query Direction found %s",id, message.c_str());
    int i = message.find("*");
    char stype = message.c_str()[1];
    stringstream ss;
    ss.str("");
    if (i > 0){
        logger->debug("[%d] [tcpClient] Query direction for all locos",id);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                ss << "M";
                ss << it->second->getCharSessionType();
                ss << "A";
                ss << it->second->getAddressType();
                ss << it->second->getLoco();
                ss << DELIM_BTLT;
                ss << "R";
                ss << (int)it->second->getDirection();
                ss << "\n";
            }
            it++;
        }
        sendToEd(ss.str());
        return;
    }

    //specific loco
    int loco = getLoco(message);
    if (sessions.find(loco) != sessions.end()){
        edSession *session = sessions[loco];
        ss.clear();ss.str();
        ss << "M";
        ss << session->getCharSessionType();
        ss << "A";
        ss << session->getAddressType();
        ss << session->getLoco();
        ss << DELIM_BTLT;
        ss << "R";
        ss << (int)session->getDirection();
        ss << "\n";
        sendToEd(ss.str());
    }
}

void tcpClient::handleQuerySpeed(string message){

    logger->debug("[%d] [tcpClient] Query speed found %s",id,message.c_str());
    int i = message.find("*");
    char stype = message.c_str()[1];
    stringstream ss;
    ss.str("");
    if (i > 0){
        logger->debug("[%d] [tcpClient] Query speed for all locos",id);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            if (stype == it->second->getCharSessionType()){
                ss << "M";
                ss << it->second->getCharSessionType();
                ss << "A";
                ss << it->second->getAddressType();
                ss << it->second->getLoco();
                ss << DELIM_BTLT;
                ss << "V";
                //if (it->second->getSpeed() == 1) ss << "0";
                //else ss << (int)it->second->getSpeed();
                ss << (int)it->second->getSpeed();
                ss << "\n";
            }
            it++;
        }
        sendToEd(ss.str());
        return;
    }

    //specific loco
    int loco = getLoco(message);
    if (sessions.find(loco) != sessions.end()){
        edSession *session = sessions[loco];
        ss.clear();ss.str();
        ss << "M";
        ss << session->getCharSessionType();
        ss << "A";
        ss << session->getAddressType();
        ss << session->getLoco();
        ss << DELIM_BTLT;
        ss << "V";
        //if (session->getSpeed() == 1) ss << "0";
        //else ss << (int)session->getSpeed();
        ss << (int)session->getSpeed();
        ss << "\n";
        sendToEd(ss.str());
    }
}

void tcpClient::handleSetFunction(string message){

    logger->debug("[%d] [tcpClient] Set function request found %s",id,message.c_str());

    // the ED always sends an on and off we will consider just the on and toggle the function internally
    //get the function

    int i = message.find(">F");
    logger->debug("[%d] [tcpClient] Extracted on/off: %s func: %s" ,id,message.substr(i+2,1).c_str(), message.substr(i+3,message.length()-(i+3)).c_str());
    int onoff = atoi(message.substr(i+2,1).c_str());
    int fn = atoi(message.substr(i+3,message.length()-(i+3)).c_str());
    i = message.find("*");
    edSession *session;
    char stype = message.c_str()[1];

    //all sessions
    if ( i > 0){
        logger->debug("[%d] [tcpClient] Setting function for all sessions",id);
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            session = it->second;
            if (stype == session->getCharSessionType()){
                if ((session->getFnType(fn) == 1) && (onoff == 0) ){
                    logger->debug("[%d] [tcpClient] Fn Message for toggle fn and for a off action. Discarding",id);
                }
                else{
                    logger->debug("[%d] [tcpClient] Previous FN state %02x" , id,session->getFnState(fn));
                    sendFnMessages(session,fn,message);
                    logger->debug("[%d] [tcpClient] Last FN state %02x" ,id, session->getFnState(fn));
                }
            }
            it++;
        }
        return;
    }
    //one session
    int loco = getLoco(message);
    if (sessions.find(loco) != sessions.end()){
        session = sessions[loco];

        if ((session->getFnType(fn) == 1) && (onoff == 0) ){
            logger->debug("[%d] [tcpClient] Fn Message for toggle fn and for a off action. Discarding",id);
        }
        else{
            sendFnMessages(session,fn,message);
        }
    }
    else logger->debug("[%d] [tcpClient] Loco %d not allocated" ,id,loco);
}

void tcpClient::handleTurnout(string message){

    /*
    #received from ed
    PTA2MT+30;-30
    #sent back
    PTA4MT+30;-30
    */

    logger->debug("[%d] [tcpClient] Turnout message found %s",id,message.c_str());

    // the ED always sends an on and off we will consider just the on and toggle the function internally
    //get the function
    char msg[CAN_MSG_SIZE];
    memset(msg,0,CAN_MSG_SIZE);

    int i = message.find("MT+");
    int j = message.find(";");
    logger->debug("[%d] [tcpClient] Extracted turnout code: %s" ,id,message.substr(i+3,j-i-3).c_str());
    int tcode = atoi(message.substr(i+3,j-i-3).c_str());

    //sanity checkings
    string scode = message.substr(i+3,j-i-3).c_str();
    if (turnouts->size() == 0){
        logger->debug("[%d] [tcpClient] Turnout table empty. Inserting turnout %d" ,id, tcode);
        turnouts->addTurnout(scode,tcode);
    }
    if (!turnouts->exists(tcode)){
        logger->debug("[%d] [tcpClient] Turnout $d does not exist. Inserting" ,id,tcode);
        turnouts->addTurnout(scode,tcode);
        if (!turnouts->exists(tcode)){
            logger->debug("[%d] [tcpClient] Failed to insert turnout %d" ,id, tcode);
        }
    }
    //prepare the data for the cbus messages
    byte Hb,Lb;
    Lb = tcode & 0xff;
    Hb = (tcode >> 8) & 0xff;
    // check the turnout state and send the short events
    if (turnouts->getTurnoutState(tcode) == TurnoutState::THROWN){
        turnouts->CloseTurnout(tcode);
        sendCbusMessage(OPC_ASOF, 0, 0 , Hb, Lb );
        //send to other EDs
        msg[0] = OPC_ASOF;
    }
    else{
        turnouts->ThrownTurnout(tcode);
        sendCbusMessage(OPC_ASON, 0, 0 , Hb, Lb );
        //send to other EDs
        msg[0] = OPC_ASON;
    }
    sendToEd(turnouts->getTurnoutMsg(tcode) + "\n");
    //send to the other clients
    msg[1] = 0;
    msg[2] = 0;
    msg[3] = Hb;
    msg[4] = Lb;
    server->postMessageToAllClients(id,can->getCanId(),msg,5,ClientType::ED);
}

void tcpClient::handleTurnoutGeneric(string message){

    /*
    #received from ed
    PTACMT8
    #sent back
    PTACMT8
    */

    logger->debug("[%d] [tcpClient] Turnout generic message found %s",id,message.c_str());

    // the ED always sends an on and off we will consider just the on and toggle the function internally
    //get the function

    int i = message.find("PTA");
    int j = message.find("MT");

    logger->debug("[%d] [tcpClient] Extracted turnout action: %s" ,id,message.substr(i+3,1).c_str());
    string tstate = message.substr(i+3,1).c_str();

    logger->debug("[%d] [tcpClient] Extracted turnout code: %s" ,id,message.substr(j+2,message.size()-j-2).c_str());
    int tcode = atoi(message.substr(j+2,message.size()-j-2).c_str());
    //sanity checkings
    string scode = message.substr(j+2,message.size()-j-2).c_str();
    if (turnouts->size() == 0){
        logger->debug("[%d] [tcpClient] Turnout table empty. Inserting turnout %d" ,id, tcode);
        turnouts->addTurnout(scode,tcode);
    }
    if (!turnouts->exists(tcode)){
        logger->debug("[%d] [tcpClient] Turnout $d does not exist. Inserting" ,id,tcode);
        turnouts->addTurnout(scode,tcode);
        if (!turnouts->exists(tcode)){
            logger->debug("[%d] [tcpClient] Failed to insert turnout %d" ,id, tcode);
        }
    }
    //prepare the data for the cbus messages
    byte Hb,Lb;
    Lb = tcode & 0xff;
    Hb = (tcode >> 8) & 0xff;
    // check the turnout state and send the short events
    if (tstate == "C"){
        turnouts->CloseTurnout(tcode);
        sendCbusMessage(OPC_ASOF, 0, 0 , Hb, Lb );
    }
    else if (tstate == "T"){
        turnouts->ThrownTurnout(tcode);
        sendCbusMessage(OPC_ASON, 0, 0 , Hb, Lb );
    }
    else {
        //toggle
        if (turnouts->getTurnoutState(tcode) == TurnoutState::THROWN){
            turnouts->CloseTurnout(tcode);
            sendCbusMessage(OPC_ASOF, 0, 0 , Hb, Lb );
        }
        else{
            turnouts->ThrownTurnout(tcode);
            sendCbusMessage(OPC_ASON, 0, 0 , Hb, Lb );
        }
    }
    sendToEd(turnouts->getTurnoutMsg(tcode) + "\n");
}

void tcpClient::sendFnMessages(edSession* session, int fn, string message){

    logger->debug("[%d] [tcpClient] Set function %d for loco %d" ,id, fn, session->getLoco());

    byte fnbyte = 1;
    byte fnbyte2 = 0;
    stringstream ss;

    //1 is F0(FL) to F4
    //2 is F5 to F8
    //3 is F9 to F12
    //4 is F13 to F19
    //5 is F20 to F28
    if ((4 < fn) && (fn < 9))    fnbyte = 2;
    if ((8 < fn) && (fn < 13))   fnbyte = 3;
    if ((12 < fn) && (fn < 20))  fnbyte = 4;
    if ((19 < fn) && (fn < 29))  fnbyte = 5;

    if (session->getFnState(fn) == 1) session->setFnState(fn,0);
    else session->setFnState(fn,1);

    //send status to ED
    int i = message.find(">F");
    logger->debug("[%d] [tcpClient] message: %s",id, message.substr(0, i+2).c_str());
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
        logger->debug("[%d] [tcpClient] Extracted loco: %s",id, msg.substr(4,i-4).c_str());
        int loco = atoi(msg.substr(4,i-4).c_str());
        return loco;
    }
    catch(...){
        return 0;
    }
}
