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
#include <string>
#include <sys/stat.h>

//logger
#include "log4cpp/Portability.hh"
#ifdef LOG4CPP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <iostream>
#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/RollingFileAppender.hh"
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

#include "libconfig.h++"

//project classes
#include "utils.h"
#include "canHandler.h"
#include "Turnout.h"
#include "nodeConfigurator.h"

using namespace std;
using namespace libconfig;
int running = 1;

void sigterm(int signo)
{
   running = 0;
}

inline bool file_exists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

int main()
{
    signal(SIGTERM, sigterm);
    signal(SIGHUP, sigterm);
    signal(SIGINT, sigterm);

    //****************
    //default config
    //***************
    log4cpp::Priority::PriorityLevel loglevel = log4cpp::Priority::DEBUG;
    string logfile = "canpi.log";
    string configfile = "canpi.cfg";
    string turnoutfile = "turnout.txt";
    int port = 30;
    string candevice = "can0";
    bool append = false;
    bool start_grid_server = false;
    int gridport = 31;
    int canid = 110;
    int pb_pin=4;
    int gled_pin=5;
    int yled_pin=6;
    int node_number=4321;
    log4cpp::Category& logger = log4cpp::Category::getRoot();
    nodeConfigurator *config = new nodeConfigurator(configfile,&logger);

    //get configuration
    string debugLevel = config->getLogLevel();

    if (!debugLevel.empty()){
        if (debugLevel.compare("INFO")== 0){
           loglevel = log4cpp::Priority::INFO;
        }
        if (debugLevel.compare("WARN")== 0){
           loglevel = log4cpp::Priority::WARN;
        }
    }

    logfile = config->getLogFile();
    candevice = config->getCanDevice();
    port = config->getTcpPort();
    canid = config->getCanID();
    gridport = config->getcanGridPort();
    append = config->getLogAppend();
    start_grid_server = config->isCanGridEnabled();
    turnoutfile = config->getTurnoutFile();
    pb_pin = config->getPB();
    gled_pin = config->getGreenLed();
    yled_pin = config->getYellowLed();
    node_number = config->getNodeNumber();

    //config the logger

    log4cpp::PatternLayout * layout1 = new log4cpp::PatternLayout();
    layout1->setConversionPattern("%d [%p] %m%n");

    log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
    appender1->setLayout(new log4cpp::BasicLayout());
    appender1->setLayout(layout1);

    log4cpp::PatternLayout * layout2 = new log4cpp::PatternLayout();
    layout2->setConversionPattern("%d [%p] %m%n");

    //log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", logfile, append);
    log4cpp::Appender *appender2 = new log4cpp::RollingFileAppender("default", logfile,5*1024*1024,10, append);//5M
    appender2->setLayout(new log4cpp::BasicLayout());
    appender2->setLayout(layout2);

    //logger.setPriority(log4cpp::Priority::DEBUG);
    logger.setPriority(loglevel);
    logger.addAppender(appender1);
    logger.addAppender(appender2);

    logger.info("Logger initated");

    config->printMemoryNVs();

    //start the components
    canHandler can = canHandler(&logger,canid);
    //set gpio pins
    can.setPins(pb_pin,gled_pin,yled_pin);
    //set the configurator
    can.setConfigurator(config);
    can.setNodeNumber(node_number);

    //start threads
    can.start(candevice.c_str());

    Turnout turnouts=Turnout(&logger);
    if (file_exists(turnoutfile)){
        turnouts.load(turnoutfile);
    }

    tcpServer tcpserver = tcpServer(&logger,port,&can,ClientType::ED);
    tcpserver.setTurnout(&turnouts);
    tcpserver.setNodeConfigurator(config);
    tcpserver.start();
    can.setTcpServer(&tcpserver);

    if (start_grid_server){
        tcpServer tcpserverGrid = tcpServer(&logger,gridport,&can,ClientType::GRID);
        tcpserverGrid.setNodeConfigurator(config);
        tcpserverGrid.start();
        can.setTcpServer(&tcpserverGrid);
    }

    while (running == 1){usleep(1000000);};

    logger.info("Stopping the tcp server");
    tcpserver.stop();

    logger.info("Stopping CBUS reader");
    can.stop();

    long t = 2 * 1000000;
    usleep(t);

    //clear the stuff
    log4cpp::Category::shutdown();
    delete config;

    return 0;
}
