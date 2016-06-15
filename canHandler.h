#ifndef CANHANDLER_H
#define CANHANDLER_H
#include <linux/can.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include <log4cpp/Category.hh>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include "tcpServer.h"
#include "utils.h"
#include "opcodes.h"

#define CAN_MSG_SIZE 8
#define WAIT_ENUM 200 //ms

using namespace std;
class tcpServer;

class canHandler
{
    public:
        canHandler(log4cpp::Category *logger,int canId);
        virtual ~canHandler();
        int start(const char* interface);
        int insert_data(char *msg,int size,ClientType ct);
        int insert_data(int canid,char *msg,int size,ClientType ct);
        void stop();
        void setCanId(int id);
        int getCanId();
        void setTcpServer(tcpServer * tcpserver);
    protected:
    private:
        int canId;
        int node_number;
        int canInterface;
        int running;
        struct ifreq ifr;
        struct sockaddr_can addr;
        log4cpp::Category *logger;
        pthread_t cbusReader;
        pthread_t queueReader;
        pthread_t queueWriter;
        tcpServer *tcpserver;
        vector<tcpServer*> servers;
        std::queue<can_frame> in_msgs;
        std::queue<can_frame> out_msgs;
        bool auto_enum_mode = false;
        vector<int> canids;
        long double sysTimeMS_start;
        long double sysTimeMS_end;

        void run_in(void* param);
        void run_out(void* param);
        void run_queue_reader(void* param);
        void doSelfEnum();
        void finishSelfEnum(int id);
        void handleCBUSEvents(struct can_frame frame);

        void print_frame(can_frame *frame,string message);

        static void* thread_entry_in(void *classPtr){
            ((canHandler*)classPtr)->run_in(nullptr);
            return nullptr;
        }
        static void* thread_entry_out(void *classPtr){
            ((canHandler*)classPtr)->run_out(nullptr);
            return nullptr;
        }
        static void* thread_entry_in_reader(void *classPtr){
            ((canHandler*)classPtr)->run_queue_reader(nullptr);
            return nullptr;
        }
};

#endif // CANHANDLER_H
