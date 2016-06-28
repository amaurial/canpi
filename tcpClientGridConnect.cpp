#include "tcpClientGridConnect.h"
#include <stdio.h>

tcpClientGridConnect::tcpClientGridConnect(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr,int id,nodeConfigurator *config)
{
    //ctor

    this->server = server;
    this->can = can;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    this->clientType = ClientType::GRID;
    logger->debug("Grid client %d created", id);
    this->config = config;
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
    stringstream ss;
    try{
        char buf[30];
        memset(buf,0,20);
        sprintf(buf,"canid:%d data:%02x %02x %02x %02x %02x %02x %02x %02x\n",canid, msg[0],msg[1],msg[2],msg[3],msg[4],msg[5],msg[6],msg[7]);
        logger->debug("[%d] Tcp grid client received cbus message: %s",id,buf);

/*
The GridConnect protocol encodes messages as an ASCII string of up to 24 characters of the form:
:ShhhhNd0d1d2d3d4d5d6d7;
The S indicates a standard CAN frame
:XhhhhhhhhNd0d1d2d3d4d5d6d7;
The X indicates an extended CAN frame hhhh is the two byte header N or R indicates a normal
or remote frame, in position 6 or 10 d0 - d7 are the (up to) 8 data bytes
*/
        memset(buf,0,30);
        byte h2 = canid << 5;
        byte h1 = canid >> 3;

        ss.clear();ss.str();
        ss << ":S";
        char t[2];
        sprintf(t,"%02X",h1);
        ss << t;
        sprintf(t,"%02X",h2);
        ss << t;
        ss << "N";
        for (int i=0;i<dlc;i++){
            sprintf(t,"%02X",msg[i]);
            ss << t;
        }
        ss << ";";
        int s = 24 - (8 - dlc)*2; // max msg + \n - data size offset
        logger->debug("[%d] Grid server sending grid message: %s",id, ss.str().c_str());
        nbytes = write(client_sock,ss.str().c_str(),s);
        if (nbytes != s){
            logger->warn("Bytes written does not match the request. Request size %d, written %d",s,nbytes);
        }
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
                logger->debug("[%d] Received from grid client:%s Bytes:%d",id, msg, nbytes);
                handleClientGridMessage(msg,nbytes);
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

void tcpClientGridConnect::handleClientGridMessage(char *msg,int size){
    vector<string> messages;
    string message(msg);
    messages = split(message,'\n', messages);
    string canid;
    string data;

    int pos,delim;

    logger->debug("[%d] Grid Processing message with size %d",id,message.size());
    vector<byte> vcanid;
    vector<byte> vdata;
    char candata[CAN_MSG_SIZE];

    for (auto const& a:messages){

        string ms(a);
        if (a.size()<4){
            continue;
        }

        pos = ms.find("S");
        if (pos>0){
            canid = ms.substr(pos+1,4);
        }
        else{
            logger->warn("[%d] Invalid grid string:[%s] no S found",id,ms.c_str());
            continue;
        }
        pos = ms.find("N");
        if (pos>0){
            delim = ms.find(";");
            if (delim <= pos){
                logger->warn("[%d] Invalid grid string:[%s] no ; found",id,ms.c_str());
                continue;
            }
            //get the chars
            data = ms.substr(pos+1,delim - pos-1);
            vcanid.clear();
            vcanid = getBytes(canid,&vcanid);
            vdata.clear();
            vdata = getBytes(data,&vdata);

            //create a can frame
            int icanid = vcanid.at(0);
            icanid = icanid<< 8;
            icanid = icanid | vcanid.at(1);
            icanid = icanid >> 5;

            int j = vdata.size()>CAN_MSG_SIZE?CAN_MSG_SIZE:vdata.size();
            memset(candata,0,CAN_MSG_SIZE);
            for (int i=0;i<j;i++){
                candata[i] = vdata.at(i);
            }

            logger->debug("Grid parsed canid:%d data:%s",icanid && 0x7f,candata);
            can->put_to_out_queue(icanid,candata,j,clientType);
            can->put_to_incoming_queue(icanid,candata,j,clientType);
            server->postMessageToAllClients(id,icanid,candata,j,clientType);
        }
        else{
            logger->warn("[%d] Invalid grid string:[%s] no N found",id,ms.c_str());
            continue;
        }
    }
}

vector<byte> tcpClientGridConnect::getBytes(string hex_chars,vector<byte> *bytes){

    //put spaces between each pair of hex

    string data;
    stringstream ss;

    logger->debug("Transform hexa bytes to byte %s",hex_chars.c_str());

    if (hex_chars.size()>2){
        for (unsigned int i=0;i<(hex_chars.size()/2);i++){
            ss << hex_chars.substr(i*2,2);
            ss << " ";
        }
        data = ss.str();
    }
    else{
        data = hex_chars;
    }
    logger->debug("Hexbytes data %s",data.c_str());

    istringstream hex_chars_stream(data);
    unsigned int c;
    while (hex_chars_stream >> std::hex >> c)
    {
        bytes->push_back((byte)(c & 0xff));
    }
    logger->debug("Hexbytes extracted %d",bytes->size());
    return *bytes;
}

