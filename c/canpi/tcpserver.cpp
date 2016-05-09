#include "tcpserver.h"

tcpserver::tcpserver(log4cpp::Category *logger)
{
    //ctor
    this->logger = logger;
}

tcpserver::~tcpserver()
{
    //dtor
}

bool tcpserver::start(){
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        logger->error("Could not create socket");
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        logger->error("Tcp server bind failed");
        return false;
    }
    return true;
}
