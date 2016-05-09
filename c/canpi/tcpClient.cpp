#include "tcpClient.h"

tcpClient::tcpClient(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr,int id)
{
    //ctor
    this->server = server;
    this->can = can;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    logger->debug("Client %d created", id);
}

tcpClient::~tcpClient()
{
    //dtor
    free(ip);
}

void tcpClient::setIp(char *ip){
    this->ip = ip;
}

char* tcpClient::getIp(){
    return ip;
}

int tcpClient::getId(){
    return id;
}

void tcpClient::start(void *param){
    running = 1;
    run(nullptr);
}

void tcpClient::stop(){
    running = 0;
}

void tcpClient::canMessage(char canid,const char* msg){
    //test to send data to client tcp
    int nbytes;
    //char buf[CAN_MSG_SIZE+1];
    char buf[30];
    memset(buf,0,20);
    nbytes = sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x\n", msg[0],msg[1],msg[2],msg[3],msg[4],msg[5],msg[6],msg[7]);
    //memcpy(buf,msg,CAN_MSG_SIZE);
    //buf[CAN_MSG_SIZE] = '\n';
    //nbytes = write(client_sock,buf,CAN_MSG_SIZE + 1);
    nbytes = write(client_sock,buf,nbytes);
}

void tcpClient::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes<1){
            logger->debug("Error while receiving data from ED");
            running = 0;
        }
        else{
            logger->debug("Received from ED:%s", msg);
        }

    }
    logger->debug("Quiting client connection %d.",id);
    close(client_sock);
}
