#ifndef TURNOUT_H
#define TURNOUT_H

#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <log4cpp/Category.hh>
#include "utils.h"
#include "msgdata.h"

using namespace std;

class Turnout
{
    public:
        Turnout(log4cpp::Category *logger);
        virtual ~Turnout();
        int load(string fileName);
        string getStartInfo();
        int getCloseCode();
        int getThrownCode();
        void CloseTurnout(int tcode);
        void ThrownTurnout(int tcode);
        TurnoutState getTurnoutState(int tcode);
        int getTurnoutCode(string tname);
        int size();
        void addTurnout(string tname,int code);
        bool exists(int tcode);
        string getTurnoutMsg(int tcode);
    protected:
    private:
        std::map<int,TurnoutState> turnouts;
        std::map<string,int> turnouts_code;
        int closed_code;
        int throw_code;
        log4cpp::Category *logger;
};

#endif // TURNOUT_H
