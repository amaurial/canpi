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

void tcpClientGridConnect::canMessage(int canid,const char* msg, int dlc){
    //test to send data to client tcp
    int nbytes;
    char buf[CAN_MSG_SIZE+1];
    stringstream ss;
    try{
        char buf[30];
        memset(buf,0,20);
        sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x\n", msg[0],msg[1],msg[2],msg[3],msg[4],msg[5],msg[6],msg[7]);
        logger->debug("[%d] Tcp grid client received cbus message: %s",id,buf);

/*
The GridConnect protocol encodes messages as an ASCII string of up to 24 characters of the form:
:ShhhhNd0d1d2d3d4d5d6d7; size = 2 + 4 + 1 + 8 + 1 = 16
The S indicates a standard CAN frame
:XhhhhhhhhNd0d1d2d3d4d5d6d7;
The X indicates an extended CAN frame hhhh is the two byte header N or R indicates a normal
or remote frame, in position 6 or 10 d0 - d7 are the (up to) 8 data bytes
*/      byte h1 = canid && 0x0000ffff;
        byte h2 = canid && 0xffff0000;
        ss << ":S";
        ss << h2 && 0xff00;
        ss << h2 && 0x00ff;
        ss << h1 && 0xff00;
        ss << h1 && 0x00ff;
        ss << "N";
        for (int a=0;a<dlc;a++){
            ss << msg[a];
        }
        ss << ";\n";

        memcpy(buf,msg,CAN_MSG_SIZE);
        int s = 17 - (8 - dlc); // max msg + \n - data size offset
        nbytes = write(client_sock,ss.str().c_str(),s);
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
            try{
                logger->debug("[%d] Received from ED:%s Bytes:%d",id, msg, nbytes);
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

