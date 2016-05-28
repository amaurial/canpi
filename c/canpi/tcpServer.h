#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <log4cpp/Category.hh>
#include <map>
#include "Client.h"
#include "canHandler.h"



class Client;

class tcpServer
{
    public:
        tcpServer(log4cpp::Category *logger, int port, canHandler* can,ClientType clientType);
        virtual ~tcpServer();
        bool start();
        void setPort(int port);
        int getPort();
        void stop();
        void removeClient(Client* client);
        void addCanMessage(int canid,const char* msg,int dlc);
        ClientType getClientType(){return clientType;};

    protected:
    private:
        canHandler *can;
        Client *tempClient;
        int running;
        int port;
        int socket_desc, client_sock ,read_size;
        struct sockaddr_in server_addr;
        int counter;
        ClientType clientType;
        log4cpp::Category *logger;
        std::map<int,Client*> clients;
        pthread_t serverThread;


        void removeClients();
        void run(void* param);
        void run_client(void* param);

        static void* thread_entry_run(void *classPtr){
            ((tcpServer*)classPtr)->run(nullptr);
            return nullptr;
        }

        static void* thread_entry_run_client(void *classPtr){
            ((tcpServer*)classPtr)->run_client(classPtr);
            return nullptr;
        }
};

#endif // TCPSERVER_H
