#include "tcpServer.h"
#include "tcpClient.h"
#include "tcpClientGridConnect.h"


tcpServer::tcpServer(log4cpp::Category *logger, int port, canHandler* can,ClientType clientType)
{
    //ctor
    this->logger = logger;
    this->setPort(port);
    this->can = can;
    counter = 0;
    this->clientType = clientType;
}

tcpServer::~tcpServer()
{
    //dtor
}

void tcpServer::setPort(int port){
    this->port = port;
}
int tcpServer::getPort(){
    return port;
}

void tcpServer::stop(){
    removeClients();
    running = 0;
}

void tcpServer::addCanMessage(int canid,const char* msg,int dlc){

    if (!clients.empty()){
        std::map<int,Client*>::iterator it = clients.begin();
        while(it != clients.end())
        {
            it->second->canMessage(canid,msg,dlc);
            it++;
        }
    }
}

bool tcpServer::start(){
    //Create socket
    logger->info("Starting tcp server on port %d",port);
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        logger->error("Could not create socket");
    }

    //Prepare the sockaddr_in structure
    int yes = 1;
    setsockopt(socket_desc,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons( port );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)
    {
        //print the error message
        logger->error("Tcp server bind failed");
        return false;
    }
    logger->debug("Start tcp listener");
    listen(socket_desc, 5);
    running = 1;

    logger->debug("Start tcp thread");

    pthread_create(&serverThread, nullptr, tcpServer::thread_entry_run, this);

    return true;
}

void tcpServer::run(void* param){
    logger->info("Waiting for connections on port %d",port);

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    logger->info("Tcp server running");

    while (running){

         client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &len);

        if (client_sock < 0)
        {
            logger->debug("Cannot accept connection");
        }
        else
        {

            char *s = NULL;
            switch(client_addr.sin_family) {
                case AF_INET: {
                    //struct sockaddr_in *addr_in = (struct sockaddr_in *)res;
                    s = (char*)malloc(INET_ADDRSTRLEN);
                    inet_ntop(AF_INET, &(client_addr.sin_addr), s, INET_ADDRSTRLEN);
                    break;
                }
                case AF_INET6: {
                    struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&client_addr;
                    s = (char*)malloc(INET6_ADDRSTRLEN);
                    inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
                    break;
                }
                default:
                    break;
            }
            logger->info("Creating client for ip:%s id:%d",s, counter);
            Client *cl;
            if (clientType == GRID){
                cl = new tcpClientGridConnect(logger,this,can,client_sock, client_addr,counter);
            }
            else{
                tcpClient *client;
                client = new tcpClient(logger,this,can,client_sock, client_addr,counter);
                client->setTurnout(turnouts);
                cl = client;
            }

            cl->setIp(s);
            free(s);
            tempClient = cl;
            logger->debug("Creating client thread %d",counter);
            pthread_t clientThread;
            pthread_create(&clientThread, nullptr, tcpServer::thread_entry_run_client, this);
            clients.insert(std::pair<int,Client*>(counter,cl));
            counter++;
        }

    }
}

void tcpServer::run_client(void* param){
    logger->info("Starting client thread %d",counter);
    tempClient->start(nullptr);
}

void tcpServer::removeClients(){

    logger->info("Stopping client connections");
    std::map<int,Client*>::iterator it = clients.begin();
    while(it != clients.end())
    {
        logger->info("Stop client %d", it->second->getId());
        it->second->stop();
        it++;
    }
    //check if it dealocattes the pointers
    clients.clear();
}

void tcpServer::removeClient(Client *client){

    if (clients.find(client->getId()) != clients.end()){
        logger->debug("Removing tcp client with id: %d",client->getId());
        clients.erase(client->getId());
    }
    else{
        logger->debug("Could not remove tcp client with id: %d",client->getId());
    }
}

void tcpServer::setTurnout(Turnout* turnouts){
    this->turnouts = turnouts;
}

