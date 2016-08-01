#include "tcpClientGridConnect.h"
#include <stdio.h>

/*
The GridConnect protocol encodes messages as an ASCII string of up to 24 characters
of the form: :ShhhhNd0d1d2d3d4d5d6d7; hhhh is the two byte (11 useful bits) header
The S indicates a standard CAN frame :XhhhhhhhhNd0d1d2d3d4d5d6d7; The X indicates
an extended CAN frame Strict Gridconnect protocol allows a variable number of header
characters, e.g., a header value of 0x123 could be encoded as S123, X123, S0123 or X00000123.
MERG hardware uses a fixed 4 or 8 byte header when sending GridConnectMessages to the computer.
The 11 bit standard header is left justified in these 4 bytes. The 29 bit standard header is
sent as <11 bit SID><0><1><0>< 18 bit EID> N or R indicates a normal or remote frame,
in position 6 or 10 d0 - d7 are the (up to) 8 data bytes
*/


tcpClientGridConnect::tcpClientGridConnect(log4cpp::Category *logger, tcpServer *server, canHandler* can, int client_sock, struct sockaddr_in client_addr,int id,nodeConfigurator *config)
{
    //ctor

    this->server = server;
    this->can = can;
    this->client_sock = client_sock;
    this->client_addr = client_addr;
    this->logger = logger;
    this->id = id;
    this->clientType = CLIENT_TYPE::GRID;
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
    int tempcanid;
    stringstream ss;
    if (running == 0){
        logger->error("Can grid client stoping. Not sending message to clients.");
        return;
    }
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
        byte h2, h1;
        int s;
        char frametype = 'N'; //should be N or R
        char t[2];
        //set frame type
        if ((canid & CAN_RTR_FLAG) == CAN_RTR_FLAG) frametype = 'R';
        //do the parse based on the extended or std frame
        if ((canid & CAN_EFF_FLAG) == CAN_EFF_FLAG){
            //extended frame
            tempcanid = canid & CAN_EFF_MASK;
            ss.clear();ss.str();
            ss << ":X";

            //first highest byte of the 4
            h1 = tempcanid >> 24;
            sprintf(t,"%02X",h1);
            ss << t;

            //second highest byte of the 4
            h1 = tempcanid >> 16;
            sprintf(t,"%02X",h1);
            ss << t;

            //third highest byte of the 4
            h1 = tempcanid >> 8;
            sprintf(t,"%02X",h1);
            ss << t;

            //fourth highest byte of the 4
            sprintf(t,"%02X",h1);
            ss << t;

            ss << frametype;
            for (int i = 0;i < dlc; i++){
                sprintf(t, "%02X", msg[i]);
                ss << t;
            }
            ss << ";";
            s = 28 - (8 - dlc)*2; // max msg + \n - data size offset
        }
        else{
            //standard frame
            tempcanid = canid & CAN_SFF_MASK;
            h2 = tempcanid << 5;
            h1 = tempcanid >> 3;

            ss.clear();ss.str();
            ss << ":S";
            sprintf(t,"%02X",h1);
            ss << t;
            sprintf(t,"%02X",h2);
            ss << t;
            ss << frametype;
            for (int i = 0; i < dlc; i++){
                sprintf(t, "%02X", msg[i]);
                ss << t;
            }
            ss << ";";
            s = 24 - (8 - dlc)*2; // max msg + \n - data size offset
        }

        logger->debug("[%d] Grid server sending grid message to client: %s",id, ss.str().c_str());
        if (running == 0){
            logger->error("Can grid client stoping. Not sending message to clients.");
            return;
        }
        nbytes = write(client_sock,ss.str().c_str(),s);
        if (nbytes != s){
            logger->warn("Bytes written does not match the request. Request size %d, written %d",s,nbytes);
        }
    }
    catch(runtime_error &ex){
        logger->debug("[%d] Grid client failed to process the can message",id);
        //throw_line("Failed to process the can message");
    }
    catch(...){
        logger->debug("[%d] Grid client failed to process the can message. Unexpected error.",id);
        //throw_line("Failed to process the can message");
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
                logger->debug("[%d] Grid client failed to process the client grid message\n%s",id,ex.what());
            }
            catch(...){
                logger->debug("[%d] Grid client failed to process the client grid message\n",id);
            }
        }
        if (nbytes == 0){
            logger->debug("[%d] Grid client 0 bytes received. disconnecting",id);
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
    bool stdframe = true;
    bool isRTR = false;

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
        //get the header
        pos = ms.find("S");
        if (pos>0){
            canid = ms.substr(pos+1,4);
        }
        else{
            //check extended frame
            pos = ms.find("X");
            if (pos>0){
                canid = ms.substr(pos+1,8);
                stdframe = false;
            }
            else{
                logger->warn("[%d] Invalid grid string:[%s] no S found",id,ms.c_str());
                continue;
            }
        }
        //get the data part
        pos = ms.find("N");
        if (pos < 1){
            //check if RTR
            pos = ms.find("R");
            if (pos > 0){
                isRTR = true;
            }
            else{
                logger->warn("[%d] Invalid grid string:[%s] no N or R found",id,ms.c_str());
                continue;
            }
        }
        if (pos>0){
            delim = ms.find(";");
            if (delim <= pos){
                logger->warn("[%d] Invalid grid string:[%s] no ; found",id,ms.c_str());
                continue;
            }
            //get the chars
            data = ms.substr(pos+1, delim - pos-1);
            vcanid.clear();
            vcanid = getBytes(canid,&vcanid);
            vdata.clear();
            vdata = getBytes(data,&vdata);

            //sanity check
            if (stdframe){
                if (vcanid.size() != 2){
                    logger->warn("[%d] Failed to parse the std grid header. Size is %d and should be 2",id,vcanid.size());
                    continue;
                }
            }
            else{
                if (vcanid.size() != 4){
                    logger->warn("[%d] Failed to parse the extended grid header. Size is %d and should be 2",id,vcanid.size());
                    continue;
                }
            }

            //create a can frame
            int icanid = vcanid.at(0);
            icanid = icanid << 8;
            icanid = icanid | vcanid.at(1);
            if (stdframe) icanid = icanid >> 5; //don't set the priority
            else{
                icanid = icanid << 8;
                icanid = icanid | vcanid.at(2);
                icanid = icanid << 8;
                icanid = icanid | vcanid.at(3);
                //set the extended frame flag
                icanid = icanid | CAN_EFF_FLAG;
            }
            //set the RTR flag
            if (isRTR) icanid = icanid | CAN_RTR_FLAG;

            //get the data
            int j = vdata.size()>CAN_MSG_SIZE?CAN_MSG_SIZE:vdata.size();
            memset(candata,0,CAN_MSG_SIZE);
            for (int i=0;i<j;i++){
                candata[i] = vdata.at(i);
            }

            logger->debug("Grid parsed canid:%d data:%s", icanid, candata);
            //put message to the wire
            can->put_to_out_queue(icanid,candata,j,clientType);
            //put message to other can clients
            can->put_to_incoming_queue(icanid,candata,j,clientType);
            //send the message to other grid clients
            server->postMessageToAllClients(id,icanid,candata,j,clientType);
        }
    }
}

vector<byte> tcpClientGridConnect::getBytes(string hex_chars,vector<byte> *bytes){

    //put spaces between each pair of hex

    string data;
    stringstream ss;

    logger->debug("Transform hexa bytes to byte %s",hex_chars.c_str());

    if (hex_chars.size() > 2){
        for (unsigned int i = 0; i < (hex_chars.size() / 2); i++){
            ss << hex_chars.substr(i*2, 2);
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

