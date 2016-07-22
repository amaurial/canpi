#include "frameCAN.h"
frameCAN::frameCAN()
{
    memset(frame.data,0,sizeof(frame.data));
    frame.can_dlc = 0;
    frame.can_id = 0;
    clientType = ClientType::CBUS;
}
frameCAN::frameCAN(struct can_frame frame,ClientType clientType)
{
    //ctor
    this->frame = frame;
    this->clientType = clientType;
}

frameCAN::~frameCAN()
{
    //dtor
}

struct can_frame frameCAN::getFrame(){
    return frame;
}
ClientType frameCAN::getClientType(){
    return clientType;
}
