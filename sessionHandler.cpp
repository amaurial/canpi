#include "sessionHandler.h"

sessionHandler::sessionHandler(log4cpp::Category *logger,nodeConfigurator *config)
{
    //ctor
    this->logger = logger;
    this->config = config;
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
        logger->debug("[sessionHandler] Allocate a new session for ed %s %d", edname.c_str(), client_id);
        sessionids++;
        edSession *ed = new edSession(logger,sessionids);
        ed->setEdName(edname);
        ed->setClientIP(client_ip);
        ed->setClientId(client_id);
        sessions.push_back(ed);
        return ed;
    }
    else{
        logger->debug("[sessionHandler] Returning existing session %d", sessions[existingsession]->getSessionUid());
        return sessions[existingsession];
    }
    return nullptr;
}

edSession* sessionHandler::retrieveEDSession(int sessionid,int client_id, string edname, long client_ip){

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
