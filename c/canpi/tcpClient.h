#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "Client.h"
#include "edSession.h"
#include "opcodes.h"
#include "msgdata.h"

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <regex>

#define BUFFER_SIZE 1024
#define CBUS_KEEP_ALIVE  4000 //ms
#define ED_KEEP_ALIVE  10000 //ms
#define BS  128
#define ST  30 //ms

using namespace std;

class tcpClient : public Client
{
    public:
        tcpClient(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr, int id);
        virtual ~tcpClient();
        void start(void *param);
        void stop();
        void canMessage(int canid,const char* msg, int dlc);
    protected:
    private:
        int running;
        edSession* edsession;
        std::map<int,edSession*> sessions; //the loco number is the key

        void run(void * param);
        void handleCBUS(const char* msg);
        void handleLearnEvents(const char* msg);
        void sendKeepAlive(void *param);
        void sendToEd(string msg);
        string generateFunctionsLabel(int loco);
        void handleEDMessages(char* msgptr);
        void handleCreateSession(string message);
        void handleReleaseSession(string message);
        void handleDirection(string message);
        void handleSpeed(string message);
        void handleQueryDirection(string message);
        void handleQuerySpeed(string message);
        void handleSetFunction(string message);
        void sendFnMessages(edSession* session, int fn, string message);
        int getLoco(string msg);
        std::vector<std::string> & split(const std::string &s, char delim, std::vector<std::string> &elems);

        void setStartSessionTime();

        regex re_speed;
        regex re_session;
        regex re_rel_session;
        regex re_dir;
        regex re_qry_speed;
        regex re_qry_direction;
        regex re_func;

        static void* thread_keepalive(void *classPtr){
            ((tcpClient*)classPtr)->sendKeepAlive(classPtr);
            return nullptr;
        }
};

#endif // TCPCLIENT_H
