#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "tcpServer.h"
#include "canHandler.h"
#include "edSession.h"
#include "opcodes.h"
#include "msgdata.h"

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include <sys/time.h>
#include <string>
#include <sstream>

#define BUFFER_SIZE 1024
#define CBUS_KEEP_ALIVE  4000 //ms
#define ED_KEEP_ALIVE  10000 //ms
#define BS  128
#define ST  30 //ms

using namespace std;

class tcpServer;
class canHandler;

class tcpClient
{
    public:
        tcpClient(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr, int id);
        virtual ~tcpClient();
        void start(void *param);
        void stop();
        void setIp(char *ip);
        string getIp();
        int getId();
        void canMessage(char canid,const char* msg);
    protected:
    private:
        tcpServer *server;
        canHandler *can;
        int client_sock;
        struct sockaddr_in client_addr;
        int running;
        int id;
        string ip;
        log4cpp::Category *logger;
        edSession* edsession;
        std::map<int,edSession*> sessions; //the loco number is the key

        void run(void * param);
        void handleCBUS(const char* msg);
        void handleLearnEvents(const char* msg);
        void sendKeepAlive();
        void sendCbusMessage(char b0=0, char b1=0,
                             char b2=0, char b3=0,
                             char b4=0, char b5=0,
                             char b6=0, char b7=0);
        void sendToEd(string msg);
        string generateFunctionsLabel(int loco);
        void handleCreateSession(string message);
        void handleReleaseSession(string message);
        void handleDirection(string message);
        int getLoco(string msg);
};

#endif // TCPCLIENT_H
