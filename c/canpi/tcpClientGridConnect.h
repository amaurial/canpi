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

#define BUFFER_SIZE 1024
/*
The GridConnect protocol encodes messages as an ASCII string of up to 24 characters of the form:
:ShhhhNd0d1d2d3d4d5d6d7;
The S indicates a standard CAN frame
:XhhhhhhhhNd0d1d2d3d4d5d6d7;
The X indicates an extended CAN frame hhhh is the two byte header N or R indicates a normal
or remote frame, in position 6 or 10 d0 - d7 are the (up to) 8 data bytes
*/

using namespace std;

class tcpClientGridConnect:public Client
{
    public:
        tcpClientGridConnect(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr, int id);
        virtual ~tcpClientGridConnect();
        void start(void *param);
        void stop();
        void canMessage(int canid,const char* msg,int dlc);
    protected:
    private:
        int running;
        void run(void * param);
        std::vector<std::string> & split(const std::string &s, char delim, std::vector<std::string> &elems);

};

#endif // TCPCLIENT_H
