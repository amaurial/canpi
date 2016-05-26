#include "tcpClientGridConnect.h"
#include <stdio.h>

tcpClientGridConnect::tcpClientGridConnect(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr,int id)
{
    //ctor

    this->server = server;
    this->can = can;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    logger->debug("Grid client %d created", id);
}

tcpClientGridConnect::~tcpClientGridConnect()
{
    //dtor
}

void tcpClientGridConnect::start(void *param){
    running = 1;
    run(nullptr);
}

void tcpClientGridConnect::stop(){
    running = 0;
}

void tcpClientGridConnect::canMessage(char canid,const char* msg){
    //test to send data to client tcp
    //int nbytes;
    //char buf[CAN_MSG_SIZE+1];
    try{
        char buf[30];
        memset(buf,0,20);
        sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x\n", msg[0],msg[1],msg[2],msg[3],msg[4],msg[5],msg[6],msg[7]);
        logger->debug("[%d] Tcp grid client received cbus message: %s",id,buf);
        //memcpy(buf,msg,CAN_MSG_SIZE);
        //buf[CAN_MSG_SIZE] = '\n';
        //nbytes = write(client_sock,buf,CAN_MSG_SIZE + 1);
        //nbytes = write(client_sock,buf,nbytes);

        handleCBUS(msg);
    }
    catch(runtime_error &ex){
        logger->debug("[%d] Failed to process the can message",id);
        throw_line("Failed to process the can message");
    }
}

void tcpClientGridConnect::run(void *param){
    char msg[BUFFER_SIZE];
    int nbytes;

    while (running){
        memset(msg,0,BUFFER_SIZE);
        nbytes = recv(client_sock, msg,BUFFER_SIZE, 0);
        if (nbytes<0){
            logger->debug("[%d] Error while receiving data from ED %d",id,nbytes);
            running = 0;
        }
        else if (nbytes>0){
            logger->debug("[%d] Received from ED:%s Bytes:%d",id, msg, nbytes);
            try{
                handleEDMessages(msg);
            }
            catch(const runtime_error &ex){
                logger->debug("[%d] Failed to process the can message\n%s",id,ex.what());
            }
        }
        if (nbytes == 0){
            logger->debug("[%d] 0 bytes received. disconnecting",id);
            running = 0;
            break;
        }
    }
    logger->info("[%d] Quiting grid client connection ip:%s id:%d.",id, ip.c_str(),id);
    usleep(1000*1000); //1sec give some time for any pending thread to finish
    close(client_sock);
    server->removeClient(this);
}

void tcpClientGridConnect::handleEDMessages(char* msgptr){
    vector<string> msgs;
    string message (msgptr);
    const char *msgtemp;

    try{
        msgs = split(message,'\n', msgs);

        vector<string>::iterator it = msgs.begin();
        for (auto const& msg:msgs){
            logger->debug("[%d] Handle message:%s",id,msg.c_str());
            if (msg.length() == 0){
                continue;
            }
            msgtemp = msg.c_str();
        }
    }
    catch(const runtime_error &ex){
        throw_line(ex.what());
    }
    catch(...){
        throw_line("Not runtime error cought");
    }
}

std::vector<std::string> & tcpClientGridConnect::split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s+' ');
    std::string item;
    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

void tcpClientGridConnect::handleCBUS(const char *msg){

    unsigned char opc = msg[0];
    int loco;
    string message;
    stringstream ss;
}

void tcpClientGridConnect::sendToEd(string msg){
    unsigned int nbytes;
    logger->debug("[%d] Send to ED:%s",id,msg.c_str());
    nbytes = write(client_sock,msg.c_str(),msg.length());
    if (nbytes != msg.length()){
        logger->error("Fail to send message %s to ED",id, msg.c_str());
    }
}
