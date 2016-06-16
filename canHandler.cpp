#include "canHandler.h"

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

canHandler::canHandler(log4cpp::Category *logger, int canId)
{
    //ctor
    this->logger = logger;
    this->canId = canId;
    this->tcpserver = nullptr;
}

canHandler::~canHandler()
{
    //dtor
}

int canHandler::getCanId(){
    return canId;
}

void canHandler::setTcpServer(tcpServer * tcpserver){
    servers.push_back(tcpserver);
}

void canHandler::setCanId(int canId){
    this->canId = canId;
}

int canHandler::insert_data(char *msg,int msize,ClientType ct){
    int c = 5; //priority 0101
    c = c << 8;
    c = c | byte(canId);
    return insert_data(c,msg,msize,ct);
}

int canHandler::insert_data(int canid,char *msg,int msize,ClientType ct){
    int i = 0;
    int j = CAN_MSG_SIZE;
    struct can_frame frame;
    if (msize < 1){
        if (canid != CAN_RTR_FLAG){
            return 0;
        }
    }

    if (msize < CAN_MSG_SIZE){
        j = msize;
    }
    memset(frame.data , 0 , sizeof(frame.data));
    frame.can_id = canid;
    frame.can_dlc = j;

    for (i = 0;i < j; i++){
        frame.data[i]=msg[i];
    }
    logger->debug("Add message to cbus queue");
    print_frame(&frame,"Insert");
    out_msgs.push(frame);

    //send to can grid server
    vector<tcpServer*>::iterator server;
    if ((servers.size() > 0) && (ct != ClientType::GRID)){
        for (server = servers.begin();server != servers.end(); server++){
            if ((*server)->getClientType() == ClientType::GRID){
                (*server)->addCanMessage(frame.can_id,(char*)frame.data, frame.can_dlc);
            }
        }
    }

    return j;
}

int canHandler::start(const char* interface){
    logger->debug("Creating socket can for %s",interface);

	if ((canInterface = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)	{
        logger->error("Unable to create can socket");
		return -1;
	}

	addr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, interface);
	ioctl(canInterface, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(canInterface, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        logger->error("Unable to bind can socket");
		return -1;
	}
	running = 1;
    logger->debug("Can socket successfuly created. Starting the CBUS reader thread");
	pthread_create(&cbusReader,nullptr,canHandler::thread_entry_in,this);

	logger->debug("Starting the queue reader thread");
	pthread_create(&queueReader,nullptr,canHandler::thread_entry_in_reader,this);

	logger->debug("Starting the queue writer thread");
	pthread_create(&queueWriter,nullptr,canHandler::thread_entry_out,this);

	return canInterface;
}

void canHandler::stop(){
    running = 0;
}

void canHandler::run_in(void* param){
    struct can_frame frame;
    int nbytes;
    struct msghdr msg;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
    struct iovec iov;

    if (canInterface <= 0){
        logger->error("Can socket not initialized");
        return;
    }

    iov.iov_base = &frame;
    msg.msg_name = &addr;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctrlmsg;

    logger->debug("Running CBUS reader");
    while (running){
        memset(frame.data , 0 , sizeof(frame.data));

        /* these settings may be modified by recvmsg() */
        iov.iov_len = sizeof(frame);
        msg.msg_namelen = sizeof(addr);
        msg.msg_controllen = sizeof(ctrlmsg);
        msg.msg_flags = 0;

        //nbytes = read(canInterface, &frame, sizeof(frame));
        nbytes = recvmsg(canInterface, &msg , 0);
        if (nbytes < 0)
        {
            logger->error("Can not read CBUS");
        }
        else{
               in_msgs.push(frame);
        }
    }
    logger->debug("Shutting down the CBUS socket");
    close(canInterface);
}

void canHandler::run_queue_reader(void* param){
    struct can_frame frame;

    logger->debug("Running CBUS queue reader");

    vector<tcpServer*>::iterator server;

    while (running){
        if (!in_msgs.empty()){
            frame = in_msgs.front();
            print_frame(&frame,"Received");

            //finish auto enum
            if (auto_enum_mode){
                if (frame.can_dlc == 0){
                    finishSelfEnum(frame.can_id);
                }
            }
            //Handle cbus
            if (frame.data[0] == OPC_QNN ||
                frame.data[0] == OPC_RQMN ||
                frame.data[0] == OPC_RQNP ||
                frame.data[0] == OPC_SNN ||
                frame.data[0] == OPC_ENUM){
                handleCBUSEvents(frame);
            }
            if (servers.size() > 0){
                for (server = servers.begin();server != servers.end(); server++){
                    (*server)->addCanMessage(frame.can_id,(char*)frame.data, frame.can_dlc);
                }
            }
            in_msgs.pop();
        }
        usleep(5000);
    }
    logger->debug("Stopping the queue reader");
}

void canHandler::print_frame(can_frame *frame,string message){

    logger->debug("%s Can Data : [%03x] [%d] data: %02x %02x %02x %02x %02x %02x %02x %02x"
                ,message.c_str()
                ,frame->can_id , frame->can_dlc
                ,frame->data[0],frame->data[1],frame->data[2],frame->data[3]
                ,frame->data[4],frame->data[5],frame->data[6],frame->data[7]);
}

void canHandler::run_out(void* param){
    struct can_frame frame;
    int nbytes;

    logger->debug("Running CBUS queue writer");

    while (running){
        if (!out_msgs.empty()){
            frame = out_msgs.front();
            //frame.can_id = canId;
            //frame.can_dlc = CAN_MSG_SIZE;
            nbytes = write(canInterface,&frame,CAN_MTU);
            print_frame(&frame,"Sent [" + to_string(nbytes) + "]");
            //logger->debug("Sent %d bytes to CBUS",nbytes);
            if (nbytes != CAN_MTU){
                logger->debug("Problem on sending the CBUS, bytes transfered %d, supposed to transfer %d", nbytes, CAN_MTU);
            }
            out_msgs.pop();
        }

        usleep(5000);

    }
    logger->debug("Stopping the queue writer");
}

void canHandler::doSelfEnum(){
    //start can enumeration
    logger->debug("Starting can enumeration");
    auto_enum_mode = true;
    struct can_frame frame;
    memset(frame.data , 0 , sizeof(frame.data));
    frame.can_id = CAN_RTR_FLAG;
    frame.can_dlc = 0;
    sysTimeMS_start = time(0)*1000;
    write(canInterface,&frame,CAN_MTU);
}

void canHandler::finishSelfEnum(int id){
    sysTimeMS_end = time(0)*1000;
    canids.push_back(id);
    if ((sysTimeMS_end - sysTimeMS_start) > WAIT_ENUM){
        logger->debug("Finishing auto enumeration.");
        auto_enum_mode = false;
        //sort and get the smallest free
        if (canids.size()>0){
            int c,n;
            canId = 0;
            sort(canids.begin(),canids.end());
            c = canids.front();
            n = 1;
            while (canId == 0){
                if (n != c){
                    canId = n;
                }
                else{
                    n++;
                    if (canids.size() > 0){
                        c = canids.front();
                    }
                    else {
                        canId = n;
                    }
                }
            }

        }
        else{
            canId = 1;
        }
        logger->debug("New canid is %d", canId);
    }
}

void canHandler::handleCBUSEvents(struct can_frame frame){

    char sendframe[CAN_MSG_SIZE];
    memset(sendframe,0,CAN_MSG_SIZE);
    byte Hb,Lb;

    switch (frame.data[0]){
    case OPC_QNN:
        Lb = node_number & 0xff;
        Hb = (node_number >> 8) & 0xff;
        sendframe[0] = OPC_PNN;
        sendframe[1] = Hb;
        sendframe[2] = Lb;
        sendframe[3] = MANU_MERG;
        sendframe[4] = MID;
        sendframe[5] = MFLAGS;
        insert_data(sendframe,6,ClientType::ED);
    break;
    case OPC_RQNP:
        sendframe[0] = OPC_PARAMS;
        sendframe[1] = MANU_MERG;
        sendframe[2] = MSOFT_MIN_VERSION;
        sendframe[3] = MID;
        sendframe[4] = 0;
        sendframe[5] = 0;
        sendframe[6] = 10;//TODO
        sendframe[7] = MSOFT_VERSION;
        insert_data(sendframe,8,ClientType::ED);
    break;
    case OPC_RQMN:
        sendframe[0] = OPC_NAME;
        sendframe[1] = 'C';
        sendframe[2] = 'A';
        sendframe[3] = 'N';
        sendframe[4] = 'W';
        sendframe[5] = 'I';
        sendframe[6] = 'F';
        sendframe[7] = 'I';
        insert_data(sendframe,8,ClientType::ED);
    break;
    case OPC_ENUM:
        //get node number
        int nn=frame.data[1];
        nn = (nn << 8) | frame.data[2];
        logger->debug("OPC_ENUM node number %d",nn);
        doSelfEnum();
    break;
    }
}

/*
OPC_QNN
case OPC_PNN:
        prepareMessageBuff(OPC_PNN,highByte(nodeId.getNodeNumber()),lowByte(nodeId.getNodeNumber()),
                            nodeId.getManufacturerId(),nodeId.getModuleId(),nodeId.getFlags());

        break;
        OPC_RQMN
    case OPC_NAME:
        prepareMessageBuff(OPC_NAME,nodeId.getNodeName()[0],nodeId.getNodeName()[1],
                            nodeId.getNodeName()[2],nodeId.getNodeName()[3],
                            nodeId.getNodeName()[4],nodeId.getNodeName()[5],
                            nodeId.getNodeName()[6]);

        break;
        OPC_RQNP
    case OPC_PARAMS:
        prepareMessageBuff(OPC_PARAMS,nodeId.getManufacturerId(),
                           nodeId.getMinCodeVersion(),nodeId.getModuleId(),
                           nodeId.getSuportedEvents(),nodeId.getSuportedEventsVariables(),
                           nodeId.getSuportedNodeVariables(),nodeId.getMaxCodeVersion());

    OPC_SNN
   prepareMessageBuff(OPC_NNACK,
highByte(nodeId.getNodeNumber()),
lowByte(nodeId.getNodeNumber())  );


*/
