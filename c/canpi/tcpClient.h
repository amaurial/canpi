#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "tcpServer.h"
#include "canHandler.h"
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>

#define BUFFER_SIZE 1024

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
        char* getIp();
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
        char* ip;
        log4cpp::Category *logger;


        void run(void * param);
};

#endif // TCPCLIENT_H
