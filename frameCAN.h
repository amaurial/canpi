#ifndef FRAMECAN_H
#define FRAMECAN_H

#include <linux/can/raw.h>
#include <string.h>
#include "utils.h"

class frameCAN
{
    public:
        frameCAN();
        frameCAN(struct can_frame frame,ClientType ctype);
        struct can_frame getFrame();
        ClientType getClientType();
        virtual ~frameCAN();
    protected:
    private:
        struct can_frame frame;
        ClientType clientType;
};

#endif // FRAMECAN_H
