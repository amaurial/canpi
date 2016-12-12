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
#include "canHandler.h"

class sessionHandler
{
    public:
        sessionHandler(log4cpp::Category *logger,nodeConfigurator *config, canHandler *can);
        virtual ~sessionHandler();
        edSession* retrieveEDSession(int client_id, string edname, long client_ip);
        unsigned int retrieveAllEDSession(int client_id, string edname, long client_ip, vector<edSession*> *edsessions);
        edSession* retrieveEDSession(int sessionid,int client_id, string edname, long client_ip);
        bool deleteEDSession(int sessionuid);
        bool deleteAllEDSessions(int client_id);
        void start(void *param);
        void stop();
    protected:
    private:
        std::vector< edSession*> sessions; //each edsession has a reference for the client
        log4cpp::Category *logger;
        nodeConfigurator *config;
        canHandler *can;
        int sessionids;
        int running;     
        pthread_t sessionHandlerThread;

        vector<edSession*> getListEDSessions(int client_id);
        unsigned int hasPendingSessions(int client_id,string edname, long client_ip);
        edSession* createEDSession(int client_id, string edname, long client_ip);
        void run(void *param);
        void sendKeepAliveForOrphanSessions();
        
        static void* run_thread_entry(void *classPtr){
            ((sessionHandler*)classPtr)->run(classPtr);
            return nullptr;
        }

};

#endif // SESSIONHANDLER_H
