#include "Server.h"

Server::Server()
{
    //ctor
    this->logger = logger;
    this->setPort(port);
    this->can = can;
    counter = 0;
}

Server::~Server()
{
    //dtor
}
