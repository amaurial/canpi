#include "sessionHandler.h"

sessionHandler::sessionHandler(log4cpp::Category *logger,nodeConfigurator *config, canHandler *can)
{
    //ctor
    this->logger = logger;
    this->config = config;
    this->can = can;
    sessionids = 0;
}

sessionHandler::~sessionHandler()
{
    //dtor
    this->sessions.clear();
}

edSession* sessionHandler::retrieveEDSession(int client_id, string edname, long client_ip){
    int existingsession = hasPendingSessions(client_id, edname, client_ip);
    if (sessions.empty() || existingsession == -1){        
        return createEDSession(client_id, edname, client_ip);
    }
    else{
        logger->debug("[sessionHandler] Returning existing session %d", sessions[existingsession]->getSessionUid());
        return sessions[existingsession];
    }
    return nullptr;
}

edSession* sessionHandler::createEDSession(int client_id, string edname, long client_ip){    
    logger->debug("[sessionHandler] Allocate a new session for ed %s %d", edname.c_str(), client_id);
    sessionids++;
    edSession *ed = new edSession(logger,sessionids);
    ed->setEdName(edname);
    ed->setClientIP(client_ip);
    ed->setClientId(client_id);
    sessions.push_back(ed);
    return ed;    
}

edSession* sessionHandler::retrieveEDSession(int sessionid, int client_id, string edname, long client_ip){

    return nullptr;
}
bool sessionHandler::deleteEDSession(int sessionuid){

    std::vector<edSession*>::iterator it = sessions.begin();
    while (it != sessions.end()){
        edSession *ed = *it;
        if (ed->getSessionUid() == sessionuid){
            sessions.erase(it);
            return true;
        }
        it++;
    }
    return false;
}

unsigned int sessionHandler::retrieveAllEDSession(int client_id, string edname, long client_ip, vector<edSession*> *edsessions){

    logger->debug("[sessionHandler] Checking for all existing sessions for ed %s %d", edname.c_str(), client_id);
    for (unsigned int i=0; i < sessions.size(); i++){
        if (sessions[i]->getClientIP() == client_ip && sessions[i]->getEdName() == edname){
            edsessions->push_back(sessions[i]);
        }
    }
    return edsessions->size();
}

bool sessionHandler::deleteAllEDSessions(int client_id){

    return false;
}

/*return the index of the first found pending session*/
unsigned int sessionHandler::hasPendingSessions(int client_id, string edname, long client_ip){

    logger->debug("[sessionHandler] Checking for existing session for ed %s %d", edname.c_str(), client_id);
    for (unsigned int i=0; i < sessions.size(); i++){
        if (sessions[i]->getClientIP() == client_ip && sessions[i]->getEdName() == edname){
            return i;
        }
    }
    return -1;
}

void sessionHandler::sendKeepAliveForOrphanSessions(){
    long millis;
    struct timespec spec;
    struct timespec t;
    
    try{
        if (sessions.size() > 0){
            
            //logger->debug("[%d] Keep alive %d sessions",id,sessions.size());
            std::map<int,edSession*>::iterator it = sessions.begin();
            while(it != sessions.end())
            {
                //logger->debug("[%d] Keep alive %d loco",id,it->second->getLoco());
                if (it->second->isOrphan()){
                    
                    clock_gettime(CLOCK_REALTIME,&spec);    

                    /*
                     * check if has elapsed more than n seconds after the session became orphan
                     * If so delete the session
                    */ 
                    
                    if (it->second->getLoco() > -1){
                        t = it->second->getCbusTime();
                        millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                        if (millis > CBUS_KEEP_ALIVE ){
                            it->second->setCbusTime(spec);
                            //send keep alive
                            logger->debug("[%d] [sessionHandler] Send CBUS keep alive",id);
                            sendCbusMessage(OPC_DKEEP,it->second->getSession());
                        }
                    }
                    else{
                        logger->debug("[%d] [sessionHandler] Loco not set keep alive",id);
                    }
                }
                it++;
            }
        }
    }//try
    catch(...){
        logger->debug("[%d] [sessionHandler] Keep alive error",id);
    }    
}

void sessionHandler::sendCbusMessage(byte b0, byte b1){
    char msg[CAN_MSG_SIZE];
    msg[0] = b0;
    msg[1] = b1;    
    logger->debug("[%d] [sessionHandler] Sending message to CBUS",id);    
    can->put_to_out_queue(msg,2,clientType);
}

void sessionHandler::start(void *param){
    running = 1;    
    pthread_create(&sessionHandlerThread, nullptr, tcpServer::run_thread_entry, this);    
}

void sessionHandler::stop(){
    running = 0;    
}

void sessionHandler::run(void *param){
    while (running){
        try{
            usleep(1000*500);//500ms
            if (running == 0){
                logger->info("[%d] [sessionHandler] Stopping keep alive process",id);
                break;
            }
            sendKeepAliveForOrphanSessions();
        }
        catch(...){
            logger->debug("[%d] [sessionHandler] Run process error",id);
        }
    } 
   
    logger->info("[%d] [sessionHandler] Finish keep alive process for orphan sessions",id);    
}