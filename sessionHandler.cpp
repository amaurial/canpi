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

edSession* sessionHandler::createEDSession(int client_id, string edname, long client_ip){
    logger->debug("[sessionHandler] Allocate a new session for ed %s %d and ip %d", edname.c_str(), client_id, client_ip);
    sessionids++;
    edSession *ed = new edSession(logger,sessionids);
    ed->setNodeConfigurator(config);
    ed->getMomentaryFNs();
    ed->setEdName(edname);
    ed->setClientIP(client_ip);
    ed->setClientId(client_id);
	ed->setOrphan(false);
    sessions.push_back(ed);
    return ed;
}

bool sessionHandler::deleteEDSession(int sessionuid){
	logger->debug("[sessionHandler] Deleting session %d", sessionuid);
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
    unsigned int i=0;
	edSession *ed;
    logger->debug("[sessionHandler] Checking for all %d existing sessions for ed %s %d and ip %d",sessions.size(), edname.c_str(), client_id, client_ip);
	std::vector<edSession*>::iterator it = sessions.begin();
    while (it != sessions.end()){
		ed = *it;    
        //if (sessions[i]->getClientIP() == client_ip && sessions[i]->getEdName() == edname){
		logger->debug("[sessionHandler] Comparing ip %d and %d",ed->getClientIP(), client_ip);
		if (ed->getClientIP() == client_ip){
            ed->setOrphan(false);
            edsessions->push_back(ed); 
			i++;           
        }
		it++;
    }
	
	logger->debug("[sessionHandler] Found %d existing sessions for %s %d", i, edname.c_str(), client_id);
	
    return i;
}

bool sessionHandler::deleteAllEDSessions(int client_id){
	bool found = false;
    std::vector<edSession*>::iterator it = sessions.begin();
    while (it != sessions.end()){
        edSession *ed = *it;
        if (ed->getClientId() == client_id){
            sessions.erase(it); 
			found = true;
        }
        it++;
    }
    return found;
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
    edSession *ed;
    try{
		//logger->info("[sessionHandler] Sessions in progress %d", sessions.size());
        if (sessions.size() > 0){

            std::vector<edSession*>::iterator it = sessions.begin();
            while(it != sessions.end())
            {
				ed = *it;
                if (ed->isOrphan()){                    
                    clock_gettime(CLOCK_REALTIME,&spec);

                    /*
                     * check if has elapsed more than n seconds after the session became orphan
                     * If so delete the session
                    */

                    t = ed->getEDTime();//the last ed time will contain the last ed keep alive
                    millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                    if (millis > timeout_orphan*1000 ){
                        //delete the session
                        logger->debug("[sessionHandler] Orphan ed timedout %d. Deleting.",ed->getSessionUid());
                        sessions.erase(it);
                        it++;
                        continue;
                    }

                    if (ed->getLoco() > -1 && ed->isSessionSet()){
                        t = ed->getCbusTime();
                        millis = spec.tv_sec*1000 + spec.tv_nsec/1.0e6 - t.tv_sec*1000 - t.tv_nsec/1.0e6;
                        if (millis > CBUS_KEEP_ALIVE ){
                            ed->setCbusTime(spec);
                            //send keep alive
                            logger->debug("[sessionHandler] Send CBUS keep alive loco [%d] session [%d]",ed->getLoco(),ed->getSession());
                            sendCbusMessage(OPC_DKEEP,ed->getSession());
                        }
                    }
                    else{
                        logger->debug("[%d] [sessionHandler] Loco not set keep alive",ed->getLoco());
                    }
                }
                it++;
            }
        }
    }//try
    catch(...){
        logger->debug("[sessionHandler] Keep alive error");
    }
}

void sessionHandler::sendCbusMessage(byte b0, byte b1){
    char msg[CAN_MSG_SIZE];
    msg[0] = b0;
    msg[1] = b1;
    logger->debug("[sessionHandler] Sending message to CBUS");
    can->put_to_out_queue(msg,2,CLIENT_TYPE::ED);
}

void sessionHandler::start(){
    running = 1;
	timeout_orphan = config->getOrphanTimeout();
    pthread_create(&sessionHandlerThread, nullptr, sessionHandler::run_thread_entry, this);
}

void sessionHandler::stop(){
    running = 0;
}

void sessionHandler::run(void *param){
    while (running){
        try{
            usleep(1000*500);//500ms
            if (running == 0){
                logger->info("[sessionHandler] Stopping keep alive process");
                break;
            }
            sendKeepAliveForOrphanSessions();
        }
        catch(...){
            logger->debug("[sessionHandler] Run process error");
        }
    }

    logger->info("[sessionHandler] Finish keep alive process for orphan sessions");
}
