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
#include "tcpServer.h"

#define CAN_MSG_SIZE 8

class tcpServer;

class canHandler
{
    public:
        canHandler(log4cpp::Category *logger,char canId);
        virtual ~canHandler();
        int start(char* interface);
        int insert_data(char *msg,int size);
        void stop();
        void setCanId(char id);
        char getCanId();
        void setTcpServer(tcpServer * tcpserver);
    protected:
    private:
        char canId;
        int canInterface;
        int running;
        struct ifreq ifr;
        struct sockaddr_can addr;
        log4cpp::Category *logger;
        pthread_t cbusReader;
        pthread_t queueReader;
        pthread_t queueWriter;
        tcpServer *tcpserver;

        std::queue<can_frame> in_msgs;
        std::queue<canfd_frame> out_msgs;

        void run_in(void* param);
        void run_out(void* param);
        void run_queue_reader(void* param);

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
