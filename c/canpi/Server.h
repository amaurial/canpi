#ifndef SERVER_H
#define SERVER_H
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include "canHandler.h"
#include "tcpClient.h"

class Client;

class Server
{
    public:
        Server();
        virtual ~Server();
        tcpServer(log4cpp::Category *logger, int port, canHandler* can);
        virtual ~tcpServer();
        bool start();
        void setPort(int port);
        int getPort(){return port;};
        void stop();
        void removeClient(Client* client);
        void addCanMessage(char canid,const char* msg);
    protected:
        void removeClients();
        void run(void* param);
        void run_client(void* param);
    private:
        canHandler *can;
        Client *tempClient;
        int running;
        int port;
        int socket_desc, client_sock ,read_size;
        struct sockaddr_in server_addr;
        int counter;
        log4cpp::Category *logger;
        std::map<int,Client*> clients;
        pthread_t serverThread;
};

#endif // TCPSERVER_H


#endif // SERVER_H
