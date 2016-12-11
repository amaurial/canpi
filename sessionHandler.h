#ifndef SESSIONHANDLER_H
#define SESSIONHANDLER_H

#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <utility>

#include "edSession.h"
#include "nodeConfigurator.h"

class sessionHandler
{
    public:
        sessionHandler(log4cpp::Category *logger,nodeConfigurator *config);
        virtual ~sessionHandler();
        edSession* retrieveEDSession(int client_id, string edname, long client_ip);
        unsigned int retrieveAllEDSession(int client_id, string edname, long client_ip, vector<edSession*> *edsessions);
        edSession* retrieveEDSession(int sessionid,int client_id, string edname, long client_ip);
        bool deleteEDSession(int sessionuid);
        bool deleteAllEDSessions(int client_id);

    protected:
    private:
        std::vector< edSession*> sessions; //each edsession has a reference for the client
        log4cpp::Category *logger;
        nodeConfigurator *config;
        int sessionids;

        vector<edSession*> getListEDSessions(int client_id);
        unsigned int hasPendingSessions(int client_id,string edname, long client_ip);

};

#endif // SESSIONHANDLER_H
