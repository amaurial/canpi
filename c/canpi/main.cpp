#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>

//logger
#include "log4cpp/Portability.hh"
#ifdef LOG4CPP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <iostream>
#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#ifdef LOG4CPP_HAVE_SYSLOG
#include "log4cpp/SyslogAppender.hh"
#endif
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/SimpleLayout.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/NDC.hh"

//project classes
#include "canHandler.h"

using namespace std;
int running = 1;

void sigterm(int signo)
{
	running = 0;
}

int main()
{

    signal(SIGTERM, sigterm);
    signal(SIGHUP, sigterm);
    signal(SIGINT, sigterm);

    log4cpp::PatternLayout * layout1 = new log4cpp::PatternLayout();
    layout1->setConversionPattern("%d [%p] %m%n");

    log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
    appender1->setLayout(new log4cpp::BasicLayout());
    appender1->setLayout(layout1);

    log4cpp::PatternLayout * layout2 = new log4cpp::PatternLayout();
    layout2->setConversionPattern("%d [%p] %m%n");
    log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", "canpi.log");
    appender2->setLayout(new log4cpp::BasicLayout());
    appender2->setLayout(layout2);


    log4cpp::Category& logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::DEBUG);
    logger.addAppender(appender1);
    logger.addAppender(appender2);

    logger.info("Logger initated");

    canHandler can = canHandler(&logger,110);
    can.start("can0");

    tcpServer tcpserver = tcpServer(&logger,5555,&can);
    tcpserver.start();

    can.setTcpServer(&tcpserver);


    while (running == 1){usleep(1000000);};

    logger.info("Stopping the tcp server");
    tcpserver.stop();

    logger.info("Stopping CBUS reader");
    can.stop();

    long t = 2 * 1000000;
    usleep(t);

    //clear the stuff
    log4cpp::Category::shutdown();

    return 0;
}
