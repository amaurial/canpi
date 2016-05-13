#include "tcpClient.h"

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
    edsession = new edSession();
}

tcpClient::~tcpClient()
{
    //dtor
    delete(edsession);
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

void tcpClient::sendCbusMessage(char b0=0, char b1=0,
                                char b2=0, char b3=0,
                                char b4=0, char b5=0,
                                char b6=0, char b7=0){
    char msg[CAN_MSG_SIZE];
    msg[0] = b0;
    msg[1] = b1;
    msg[2] = b2;
    msg[3] = b3;
    msg[4] = b4;
    msg[5] = b5;
    msg[6] = b6;
    msg[7] = b7;
    can->insert_data(msg,CAN_MSG_SIZE);
}

void tcpClient::canMessage(char canid,const char* msg){
    //test to send data to client tcp
    //int nbytes;
    //char buf[CAN_MSG_SIZE+1];
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

void tcpClient::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes<1){
            logger->debug("Error while receiving data from ED");
            running = 0;
        }
        else{
            logger->debug("Received from ED:%s", msg);
        }
        sendKeepAlive();
    }
    logger->debug("Quiting client connection ip:%s id:%d.",ip.c_str(),id);
    close(client_sock);
}

void tcpClient::sendKeepAlive(){
    std::map<int,edSession*>::iterator it = sessions.begin();
    long millis,t;
    timeval tv;

    while(it != sessions.end())
    {
        gettimeofday(&tv,0);
        millis = tv.tv_usec * 1000;
        t = it->second->getEDTime();
        if ((millis - t) > ED_KEEP_ALIVE ){
            it->second->setEDTime(millis);
            //send to ED
            sendToEd("\n");
        }

        t = it->second->getCbusTime();
        if ((millis - t) > CBUS_KEEP_ALIVE ){
            it->second->setCbusTime(millis);
            //send keep alive
            sendCbusMessage(OPC_DKEEP,it->second->getSession(),0,0,0,0,0,0);
        }
        it++;
    }
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

        speed = msg[4] & 0x7F; //#0111 1111
        direction = 0;
        if ((msg[4] & 0x80) > 127)   direction = 1;

        //put session in array
        edsession->setDirection(direction);
        edsession->setSpeed(speed);
        logger->debug("Adding loco %d to sessions",  loco);
        sessions.insert(std::pair<int,edSession*>(loco,edsession));
        edsession = new edSession();

        ss << "MT+";
        ss << edsession->getAddressType();
        ss << loco;
        ss << DELIM_BTLT;
        ss << '\n';

        message = ss.str();
        logger->debug("Ack client session created %d for loco %d :%s" ,session, loco, message.c_str());
        sendToEd(message);

        //set speed mode 128 to can
        sendCbusMessage(OPC_STMOD, session,0,0,0,0,0,0);

        //send the labels to client
        ss.clear();ss.str("");
        ss << "MTLS";
        ss << loco;
        ss << EMPTY_LABELS;
        ss << "S";
        ss << generateFunctionsLabel(loco);
        ss << '\n';

        logger->debug("Sending labels");

        sendToEd(ss.str());

        logger->debug("Sending speed mode 128");
        ss.clear();ss.str("");
        ss << "MT";
        ss << edsession->getAddressType();
        ss << loco;
        ss << DELIM_BTLT;
        ss << "s0\n";

        sendToEd(ss.str());
        break;
    case OPC_ERR:
        logger->debug("OPC: ERR");
        loco = msg[2] & 0x3f;
        loco = (loco << 8) + msg[3];

        if (loco != edsession->getLoco()){
            logger->debug("Error message for another client. Different loco number. Discarding.");
            return;
        }

        switch (msg[3]){
        case 1:
            logger->debug("Can not create session. Reason: stack full");
            break;
        case 2:
            logger->debug("Err: Loco %d TAKEN", loco);
            break;
        case 3:
            logger->debug("Err: No session %d" , loco);
            break;
        default:
            logger->debug("Err code: %d" , msg[3]);
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
    int nbytes;
    nbytes = write(client_sock,msg.c_str(),msg.length());
    if (nbytes != msg.length()){
        logger->error("Fail to send message %s to ED", msg.c_str());
    }
}

void tcpClient::handleCreateSession(string message){
    const char *msg = message.c_str();
    logger->debug("Handle create session. MT+ found");
    unsigned char Hb,Lb;
    int loco;
    //create session
    if ( (msg[3] == 'S') | (msg[3] == 's') | (msg[3] == 'L') | (msg[3] == 'l') ){

        logger->debug("MT+ found");
        edsession->setAddressType(msg[3]);
        logger->debug("Address type: %s", edsession->getAddressType());
        //get loco
        loco = getLoco(message);
        edsession->setLoco(loco);
        //send the can data
        logger->debug("Put CAN session request in the queue for loco %d" , loco);

        Hb = 0;
        if ((loco > 127) | (msg[3] == 'L') | (msg[3] == 'l')){
            Hb = loco >> 8 | 0xC0;
            Lb = loco & 0xFF;
        }
        else Lb = loco & 0xFF;

        sendCbusMessage(OPC_RLOC,Hb,Lb,0,0,0,0,0);
    }
}


void tcpClient::handleReleaseSession(string message){

    logger->debug("Handle release session. MT- found");
    //release session
    sendToEd(message + "\n");
    int i = message.find("*");

    //all sessions
    if (i>0){
        logger->debug("Releasing all sessions");
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            logger->debug("Releasing session for loco KLOC %d" , it->second->getLoco());
            sendCbusMessage(OPC_KLOC,it->second->getSession());
            usleep(5000);//5ms
            it++;
        }
        //clear sessions
        sessions.clear();
        return;
    }

    int loco = getLoco(message);
    logger->debug("Releasing session for loco KLOC %d" , loco);

    char sesid = sessions[loco]->getSession();
    //send the can data
    sendCbusMessage(OPC_KLOC,sesid);
    sessions.erase(loco);
}


void tcpClient::handleDirection(string message){
    const char* msg = message.c_str();

    logger->debug("Handle Direction request");
    sendToEd(message + "\n");

    //get the direction
    int i = message.find(">R");
    logger->debug("Extracted direction: %s" ,  message.substr(i+2,1).c_str());
    int d = atoi(message.substr(i+2,1).c_str());

    i = message.find("*");
    //all sessions
    if (i > 0){
        logger->debug("Set direction for all sessions");
        std::map<int,edSession*>::iterator it = sessions.begin();
        while(it != sessions.end())
        {
            if (d != it->second->getDirection()){
                it->second->setDirection(d);
                logger->debug("Set direction %d for loco %d" , d ,it->second->getLoco());
                sendCbusMessage(OPC_DSPD,it->second->getSession(),d*BS+it->second->getSpeed());
            }
            it++;
        }
        return;
    }

    //one session
    int loco = getLoco(message);
    int sesid = sessions[loco]->getSession();

    if (d != sesid){
        sessions[loco]->setDirection(d);
        logger->debug("Set direction %d for loco %d" , d ,loco);
        sendCbusMessage(OPC_DSPD,sesid, d * BS + sessions[loco]->getSpeed());
    }

}

void tcpClient::handleSpeedDir(string message){

    const char* msg message.c_str();
    string speedString;

    logger->debug("Handle speed request")
    sendToEd("\n");

    int i = message.find(">V");
    if (i > 0){
        int s = i + 1;
        logger->debug("Extracted speed: %s", message.substr(s,message.length()-s));
        speedString = message.substr(s,message.length()-s);
    }
    else{
        i = message.find(">X");
        if (i > 0) speedString = "X";
        else{
            logger->debug("Bad speed message format. Discarding.")
            return;
        }
    }
    int speed = 0;

    if (speedString =='X') speed = 1;
    else{
        speed = atoi(speedString.c_str());
        if (speed != 0) speed++;
    }
    i = message.find("*");
    //all sessions
    if i > 0:
        logger->debug("Set speed for all sessions")
        for k, s in self.sessions.items():
            if s:
                logger->debug("Set speed %d for loco %d" % (speed, s.getLoco()))
                self.sessions.get(s.getLoco()).setSpeed(speed)
                sdir = s.getDirection() * BS + speed
                self.can.put(OPC_DSPD + bytes([s.getSessionID()]) + bytes([sdir]))
        return

    #one session
    loco = self.getLoco(msg)
    session = self.sessions.get(loco)

    if session:
        #send the can data
        logger->debug("Set speed %d for loco %d" % (speed, loco))
        self.sessions.get(loco).setSpeed(speed)
        sdir = session.getDirection() * BS + speed
        self.can.put(OPC_DSPD + bytes([session.getSessionID()]) + bytes([sdir]))
    else:
        logger->debug("No session found for loco %d" % loco)
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
