#ifndef TCPCLIENTGRID_H
#define TCPCLIENTGRID_H

#include "Client.h"
#include "canHandler.h"
#include "opcodes.h"
#include "msgdata.h"
#include "utils.h"

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

using namespace std;

class tcpClientGridConnect:public Client
{
    public:
        tcpClientGridConnect(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr, int id);
        virtual ~tcpClientGridConnect();
        void start(void *param);
        void stop();
        void canMessage(char canid,const char* msg);
    protected:
    private:
        int running;
        void run(void * param);
        void handleCBUS(const char* msg);
        void sendToEd(string msg);
        void handleEDMessages(char* msgptr);
        std::vector<std::string> & split(const std::string &s, char delim, std::vector<std::string> &elems);

};

#endif // TCPCLIENT_H
