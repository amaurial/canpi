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

int canHandler::insert_data(char *msg,int size){
    int i = 0;
    int j = CAN_MSG_SIZE;
    struct canfd_frame frame;
    if (size < 1){
        return 0;
    }

    if (size < CAN_MSG_SIZE){
        j = size;
    }
    memset(frame.data , 0 , sizeof(frame.data));
    frame.can_id = canId;
    frame.len = j;

    for (i = 0;i < j; i++){
        frame.data[i]=msg[i];
    }
    logger->debug("Add message %s to cbus queue", msg);
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
        //if ((nbytes = read(canInterface, &frame, sizeof(frame))) < 0)
        nbytes = recvmsg(canInterface, &msg , 0);
        if (nbytes < 0)
        {
            logger->error("Can not read CBUS");
            return;
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
            if (tcpserver != nullptr){
                tcpserver->addCanMessage(frame.can_id,(char*)frame.data);
            }

            logger->debug("Recv Can Data : [%03x] %02x %02x %02x %02x %02x %02x %02x %02x"
                ,frame.can_id
                ,frame.data[0],frame.data[1],frame.data[2],frame.data[3]
                ,frame.data[4],frame.data[5],frame.data[6],frame.data[7]);
            in_msgs.pop();
        }
        usleep(10000);
    }
    logger->debug("Stopping the queue reader");
}

void canHandler::run_out(void* param){
    struct canfd_frame frame;

    logger->debug("Running CBUS queue writer");
    while (running){
        if (!out_msgs.empty()){
            frame = out_msgs.front();
            logger->debug("Sent Can Data : [%03x] %02x %02x %02x %02x %02x %02x %02x %02x"
                ,frame.can_id
                ,frame.data[0],frame.data[1],frame.data[2],frame.data[3]
                ,frame.data[4],frame.data[5],frame.data[6],frame.data[7]);
            out_msgs.pop();

        }
        usleep(10000);
    }
    logger->debug("Stopping the queue writer");
}
