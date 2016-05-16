#include "canHandler.h"

canHandler::canHandler(log4cpp::Category *logger, char canId)
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

char canHandler::getCanId(){
    return canId;
}

void canHandler::setTcpServer(tcpServer * tcpserver){
    this->tcpserver = tcpserver;
}

void canHandler::setCanId(char canId){
    this->canId = canId;
}

int canHandler::insert_data(char *msg,int msize){
    int i = 0;
    int j = CAN_MSG_SIZE;
    struct can_frame frame;
    if (msize < 1){
        return 0;
    }

    if (msize < CAN_MSG_SIZE){
        j = msize;
    }
    memset(frame.data , 0 , sizeof(frame.data));
    frame.can_id = canId;
    frame.can_dlc = j;

    for (i = 0;i < j; i++){
        frame.data[i]=msg[i];
    }
    logger->debug("Add message to cbus queue");
    print_frame(&frame,"Insert");
    out_msgs.push(frame);
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
    while (running){
        if (!in_msgs.empty()){
            frame = in_msgs.front();
            print_frame(&frame,"Received");
            if (tcpserver != nullptr){
                tcpserver->addCanMessage(frame.can_id,(char*)frame.data);
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
            frame.can_id = canId;
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
