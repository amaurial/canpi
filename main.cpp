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
#include "canHandler.h"
#include "Turnout.h"
#define INTERROR 323232

using namespace std;
using namespace libconfig;
int running = 1;

void sigterm(int signo)
{
   running = 0;
}

/**
* Usefull function to get string from config file
**/
string getStringCfgVal(Config *cfg,string key)
{
  string ret;
  try
  {
     cfg->lookupValue(key,ret);
  }
  catch(const SettingNotFoundException &nfex)
  {
  }
  return ret;

}
/**
* Usefull function to get integer from config file
**/
int getIntCfgVal(Config *cfg,string key)
{
  int ret;
  try
  {
     bool r = cfg->lookupValue(key,ret);
     if (!r) ret = INTERROR;
  }
  catch(const SettingNotFoundException &nfex)
  {
     ret = INTERROR;
  }
  return ret;
}

inline bool file_exists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

int main()
{
    bool hasconfig = true;
    signal(SIGTERM, sigterm);
    signal(SIGHUP, sigterm);
    signal(SIGINT, sigterm);

    Config cfg;
    try
    {
       cfg.readFile("canpi.cfg");
    }
    catch(const FileIOException &fioex)
    {
	std::cerr << "File I/O error" << std::endl;
	hasconfig = false;
    }
    catch(const ParseException &pex)
    {
	std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
	hasconfig = false;
    }
    //****************
    //default config
    //***************
    log4cpp::Priority::PriorityLevel loglevel = log4cpp::Priority::DEBUG;
    string logfile = "canpi.log";
    string turnoutfile = "turnout.txt";
    int port = 5555;
    string candevice = "can0";
    bool append = false;
    bool start_grid_server = false;
    int gridport = 4444;
    int canid = 110;
    if (hasconfig){

        string debugLevel = getStringCfgVal(&cfg,"loglevel");

        if (!debugLevel.empty()){
	    if (debugLevel.compare("INFO")== 0){
	       loglevel = log4cpp::Priority::INFO;
	    }
	    if (debugLevel.compare("WARN")== 0){
	       loglevel = log4cpp::Priority::WARN;
	    }
        }

        logfile = getStringCfgVal(&cfg,"logfile");
        if (logfile.empty()){
            cout << "Failed to get log file name. Defaul is canpi.log" << endl;
            logfile = "canpi.log";
        }

        candevice= getStringCfgVal(&cfg,"candevice");
        if (logfile.empty()){
            cout << "Failed to get can device. Defaul is can0" << endl;
            logfile = "can0";
        }

        port = getIntCfgVal(&cfg,"tcpport");
        if (port == INTERROR){
                cout << "Failed to get tcp port. Defaul is 5555" << endl;
            port = 5555;
        }

        canid = getIntCfgVal(&cfg,"canid");
        if (canid == INTERROR){
                cout << "Failed to get canid. Default is 110" << endl;
            canid = 110;
        }

        gridport = getIntCfgVal(&cfg,"cangrid_port");
        if (gridport == INTERROR){
                cout << "Failed to get tcp port. Defaul is 5555" << endl;
            gridport = 5555;
        }

        string sappend= getStringCfgVal(&cfg,"logappend");
        if (sappend.empty()){
            cout << "Failed to get logappend . Defaul is false" << endl;
            append = false;
        }
        if ((sappend.compare("true") == 0) | (sappend.compare("TRUE") == 0) | (sappend.compare("True") == 0)){
            append = true;
        }

        string grid= getStringCfgVal(&cfg,"can_grid");
        if (grid.empty()){
            cout << "Failed to get can_grid . Defaul is false" << endl;
            start_grid_server = false;
        }
        if ((grid.compare("true") == 0) | (grid.compare("TRUE") == 0) | (grid.compare("True") == 0)){
            start_grid_server = true;
        }

        turnoutfile = getStringCfgVal(&cfg,"turnout_file");
        if (turnoutfile.empty()){
            cout << "Failed to get turnout file name. Defaul is turnout.txt" << endl;
            turnoutfile = "turnout.txt";
        }
    }

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

    log4cpp::Category& logger = log4cpp::Category::getRoot();
    //logger.setPriority(log4cpp::Priority::DEBUG);
    logger.setPriority(loglevel);
    logger.addAppender(appender1);
    logger.addAppender(appender2);

    logger.info("Logger initated");

    canHandler can = canHandler(&logger,canid);
    can.start(candevice.c_str());

    Turnout turnouts=Turnout(&logger);
    if (file_exists(turnoutfile)){
        turnouts.load(turnoutfile);
    }

    tcpServer tcpserver = tcpServer(&logger,port,&can,ClientType::ED);
    tcpserver.setTurnout(&turnouts);
    tcpserver.start();
    can.setTcpServer(&tcpserver);

    if (start_grid_server){
        tcpServer tcpserverGrid = tcpServer(&logger,gridport,&can,ClientType::GRID);
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

    return 0;
}
